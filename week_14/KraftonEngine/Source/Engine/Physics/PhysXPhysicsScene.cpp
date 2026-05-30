#include "Physics/PhysXPhysicsScene.h"

#include "Component/PrimitiveComponent.h"
#include "Core/ProjectSettings.h"
#include "Core/Logging/Log.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"
#include "Object/Object.h"
#include "Object/Ptr/WeakObjectPtr.h"
#include "Physics/PhysicsBodyInstance.h"
#include "Physics/PhysicsShapeInstance.h"
#include "Physics/PhysXConversion.h"

#include <algorithm>
#include <mutex>
#include <PxPhysicsAPI.h>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace physx;

// ============================================================
// PhysX Error Callback
// ============================================================
class FPhysXErrorCallback : public PxErrorCallback
{
public:
    void reportError(PxErrorCode::Enum code, const char* message,
        const char* file, int line) override
    {
        const char* severity = "Info";
        if (code == PxErrorCode::eABORT || code == PxErrorCode::eOUT_OF_MEMORY)
        {
            severity = "Fatal";
        }
        else if (code == PxErrorCode::eINTERNAL_ERROR || code == PxErrorCode::eINVALID_OPERATION)
        {
            severity = "Error";
        }
        else if (code == PxErrorCode::eINVALID_PARAMETER || code == PxErrorCode::ePERF_WARNING)
        {
            severity = "Warning";
        }
        else if (code == PxErrorCode::eDEBUG_WARNING)
        {
            severity = "Warning";
        }

        UE_LOG("[PhysX %s] %s (%s:%d)", severity, message, file, line);
    }
};

static FPhysXErrorCallback GPhysXErrorCallback;
static PxDefaultAllocator GPhysXAllocator;

// ============================================================
// PhysX Foundation/Physics singleton
// ============================================================
static PxFoundation* GSharedFoundation = nullptr;
static PxPhysics* GSharedPhysics = nullptr;
static int32 GSharedRefCount = 0;

static void AcquireSharedPhysX(PxFoundation*& OutFoundation, PxPhysics*& OutPhysics)
{
    if (GSharedRefCount == 0)
    {
        GSharedFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, GPhysXAllocator, GPhysXErrorCallback);
        if (GSharedFoundation)
        {
            GSharedPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *GSharedFoundation, PxTolerancesScale());
        }
    }

    ++GSharedRefCount;
    OutFoundation = GSharedFoundation;
    OutPhysics = GSharedPhysics;
}

static void ReleaseSharedPhysX()
{
    if (GSharedRefCount <= 0)
    {
        return;
    }

    --GSharedRefCount;
    if (GSharedRefCount == 0)
    {
        if (GSharedPhysics)
        {
            GSharedPhysics->release();
            GSharedPhysics = nullptr;
        }
        if (GSharedFoundation)
        {
            GSharedFoundation->release();
            GSharedFoundation = nullptr;
        }
    }
}

namespace
{
    FShapeInstance* GetShapeInstance(const PxShape* Shape)
    {
        return Shape && Shape->userData
            ? static_cast<FShapeInstance*>(Shape->userData)
            : nullptr;
    }

    FBodyInstance* GetBodyInstance(const PxRigidActor* Actor)
    {
        return Actor && Actor->userData
            ? static_cast<FBodyInstance*>(Actor->userData)
            : nullptr;
    }

    UPrimitiveComponent* GetComponentFromShape(const PxShape* Shape)
    {
        FShapeInstance* ShapeInstance = GetShapeInstance(Shape);
        if (!ShapeInstance || ShapeInstance->State != EPhysicsRuntimeObjectState::Alive)
        {
            return nullptr;
        }
        return ShapeInstance->SourceComponent.Get();
    }

    AActor* GetOwnerActorFromShape(const PxShape* Shape)
    {
        UPrimitiveComponent* Comp = GetComponentFromShape(Shape);
        return IsValid(Comp) ? Comp->GetOwner() : nullptr;
    }

    AActor* GetOwnerActorFromActor(const PxRigidActor* Actor)
    {
        FBodyInstance* Body = GetBodyInstance(Actor);
        return Body ? Body->OwnerActor.Get() : nullptr;
    }
}

// ============================================================
// PhysX Simulation Event Callback
// ============================================================
class FPhysXSimulationCallback : public PxSimulationEventCallback
{
public:
    struct FQueuedHit
    {
        TWeakObjectPtr<UPrimitiveComponent> Self;
        TWeakObjectPtr<UPrimitiveComponent> Other;
        FVector                             NormalImpulse{0, 0, 0};
        FVector                             WorldLocation { 0, 0, 0 };
        FVector                             WorldNormal { 0, 0, 1 };
        float                               PenetrationDepth = 0.0f;
        bool                                bBegin           = true;
    };

    struct FQueuedTrigger
    {
        TWeakObjectPtr<UPrimitiveComponent> Self;
        TWeakObjectPtr<UPrimitiveComponent> Other;
        bool                                bBegin = true;
    };

    struct FPairKey
    {
        uint32 A = 0;
        uint32 B = 0;

        FPairKey() = default;

        FPairKey(UPrimitiveComponent* InA, UPrimitiveComponent* InB)
        {
            A = IsAliveObject(InA) ? InA->GetUUID() : 0;
            B = IsAliveObject(InB) ? InB->GetUUID() : 0;
            if (A > B)
            {
                std::swap(A, B);
            }
        }

        bool IsValid() const
        {
            return A != 0 && B != 0 && A != B;
        }

        bool operator==(const FPairKey& Other) const
        {
            return A == Other.A && B == Other.B;
        }
    };

    struct FPairRecord
    {
        TWeakObjectPtr<UPrimitiveComponent> A;
        TWeakObjectPtr<UPrimitiveComponent> B;
    };

    struct FPairKeyHash
    {
        size_t operator()(const FPairKey& Key) const
        {
            size_t H = std::hash<uint32>()(Key.A);
            H        ^= std::hash<uint32>()(Key.B) + 0x9e3779b9 + (H << 6) + (H >> 2);
            return H;
        }
    };

    void onContact(const PxContactPairHeader& PairHeader,
        const PxContactPair* Pairs, PxU32 Count) override
    {
        if (PairHeader.flags & PxContactPairHeaderFlag::eREMOVED_ACTOR_0 ||
            PairHeader.flags & PxContactPairHeaderFlag::eREMOVED_ACTOR_1)
        {
            return;
        }

        for (PxU32 i = 0; i < Count; ++i)
        {
            const PxContactPair& CP = Pairs[i];
            const bool bBegin = CP.events.isSet(PxPairFlag::eNOTIFY_TOUCH_FOUND);
            const bool bEnd = CP.events.isSet(PxPairFlag::eNOTIFY_TOUCH_LOST);
            if (!bBegin && !bEnd)
            {
                continue;
            }

            UPrimitiveComponent* CompA = GetComponentFromShape(CP.shapes[0]);
            UPrimitiveComponent* CompB = GetComponentFromShape(CP.shapes[1]);
            if (!IsValid(CompA) || !IsValid(CompB))
            {
                continue;
            }

            const ECollisionResponse MinResponse = UPrimitiveComponent::GetMinResponse(CompA, CompB);
            if (MinResponse == ECollisionResponse::Ignore)
            {
                continue;
            }

            if (MinResponse == ECollisionResponse::Overlap)
            {
                if (bBegin)
                {
                    QueueOverlapPair(CompA, CompB, true);
                }
                if (bEnd)
                {
                    QueueOverlapPair(CompA, CompB, false);
                }
                continue;
            }

            PxContactPairPoint ContactPoints[1];
            PxU32 NumPoints = CP.extractContacts(ContactPoints, 1);

            FVector ContactPos(0, 0, 0);
            FVector ContactNormal(0, 0, 1);
            float Penetration = 0.0f;

            if (NumPoints > 0)
            {
                ContactPos = ToFVector(ContactPoints[0].position);
                ContactNormal = ToFVector(ContactPoints[0].normal);
                Penetration = ContactPoints[0].separation;
            }

            // PhysX contact separation is not an impulse. Until contact impulse extraction is
            // wired, expose zero impulse rather than a misleading penetration-derived value.
            const FVector NormalImpulse = FVector::ZeroVector;

            if (bBegin)
            {
                QueueHitPair(CompA, CompB, true, ContactPos, ContactNormal, NormalImpulse, -Penetration);
            }
            if (bEnd)
            {
                QueueHitPair(CompA, CompB, false, FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector, 0.0f);
            }
        }
    }

    void onTrigger(PxTriggerPair* Pairs, PxU32 Count) override
    {
        for (PxU32 i = 0; i < Count; ++i)
        {
            const PxTriggerPair& TP = Pairs[i];

            if (TP.flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
            {
                // The component unregister path emits synthetic end events from ActiveOverlapPairs.
                continue;
            }

            UPrimitiveComponent* TriggerComp = GetComponentFromShape(TP.triggerShape);
            UPrimitiveComponent* OtherComp = GetComponentFromShape(TP.otherShape);
            if (!IsValid(TriggerComp) || !IsValid(OtherComp))
            {
                continue;
            }

            const ECollisionResponse MinResponse = UPrimitiveComponent::GetMinResponse(TriggerComp, OtherComp);
            if (MinResponse == ECollisionResponse::Ignore)
            {
                continue;
            }

            const bool bBegin = TP.status == PxPairFlag::eNOTIFY_TOUCH_FOUND;
            const bool bEnd = TP.status == PxPairFlag::eNOTIFY_TOUCH_LOST;
            if (bBegin)
            {
                QueueOverlapPair(TriggerComp, OtherComp, true);
            }
            if (bEnd)
            {
                QueueOverlapPair(TriggerComp, OtherComp, false);
            }
        }
    }

    void NotifyComponentUnregistered(UPrimitiveComponent* Component)
    {
        if (!Component)
        {
            return;
        }

        std::lock_guard<std::mutex> Lock(EventMutex);
        QueueSyntheticEndEventsForComponent_NoLock(Component, ActiveOverlapPairs, PendingTriggers, true);
        QueueSyntheticEndEventsForComponent_NoLock(Component, ActiveHitPairs, PendingHits, false);
    }

    void DispatchPendingEvents(bool bDispatchHitEvents, bool bDispatchTriggerEvents)
    {
        std::vector<FQueuedHit>     HitsToDispatch;
        std::vector<FQueuedTrigger> TriggersToDispatch;
        {
            std::lock_guard<std::mutex> Lock(EventMutex);
            HitsToDispatch.swap(PendingHits);
            TriggersToDispatch.swap(PendingTriggers);
        }

        if (bDispatchHitEvents)
        {
            for (FQueuedHit& E : HitsToDispatch)
            {
                UPrimitiveComponent* Self  = E.Self.Get();
                UPrimitiveComponent* Other = E.Other.Get();
                if (!IsValid(Self) || !IsValid(Other))
                {
                    continue;
                }

                AActor* OtherActor = Other->GetOwner();
                if (E.bBegin)
                {
                    FHitResult Hit;
                    Hit.bHit             = true;
                    Hit.HitComponent     = Other;
                    Hit.HitActor         = OtherActor;
                    Hit.WorldHitLocation = E.WorldLocation;
                    Hit.ImpactNormal     = E.WorldNormal;
                    Hit.WorldNormal      = E.WorldNormal;
                    Hit.PenetrationDepth = E.PenetrationDepth;
                    Self->NotifyComponentHit(Self, OtherActor, Other, E.NormalImpulse, Hit);
                }
                else
                {
                    Self->NotifyComponentEndHit(Self, OtherActor, Other);
                }
            }
        }

        if (bDispatchTriggerEvents)
        {
            for (FQueuedTrigger& E : TriggersToDispatch)
            {
                UPrimitiveComponent* Self  = E.Self.Get();
                UPrimitiveComponent* Other = E.Other.Get();
                if (!IsValid(Self) || !IsValid(Other))
                {
                    continue;
                }

                AActor* OtherActor = Other->GetOwner();
                if (E.bBegin)
                {
                    FHitResult DummyHit;
                    DummyHit.HitComponent = Other;
                    DummyHit.HitActor     = OtherActor;
                    Self->NotifyComponentBeginOverlap(Self, OtherActor, Other, 0, false, DummyHit);
                }
                else
                {
                    Self->NotifyComponentEndOverlap(Self, OtherActor, Other, 0);
                }
            }
        }
    }

    void onConstraintBreak(PxConstraintInfo*, PxU32) override {}
    void onWake(PxActor**, PxU32) override {}
    void onSleep(PxActor**, PxU32) override {}
    void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) override {}

private:
    void QueueOverlapPair(UPrimitiveComponent* A, UPrimitiveComponent* B, bool bBegin)
    {
        if (!IsValid(A) || !IsValid(B))
        {
            return;
        }

        std::lock_guard<std::mutex> Lock(EventMutex);
        FPairKey                    Key(A, B);
        if (!Key.IsValid())
        {
            return;
        }

        if (bBegin)
        {
            if (ActiveOverlapPairs.find(Key) != ActiveOverlapPairs.end())
            {
                return;
            }
            ActiveOverlapPairs[Key] = FPairRecord { A, B };
        }
        else
        {
            auto It = ActiveOverlapPairs.find(Key);
            if (It == ActiveOverlapPairs.end())
            {
                return;
            }
            ActiveOverlapPairs.erase(It);
        }

        if (A->GetGenerateOverlapEvents()) PendingTriggers.push_back({ A, B, bBegin });
        if (B->GetGenerateOverlapEvents()) PendingTriggers.push_back({ B, A, bBegin });
    }

    void QueueHitPair(
        UPrimitiveComponent* A,
        UPrimitiveComponent* B,
        bool                 bBegin,
        const FVector&       WorldLocation,
        const FVector&       WorldNormal,
        const FVector&       NormalImpulse,
        float                PenetrationDepth
    )
    {
        if (!IsValid(A) || !IsValid(B))
        {
            return;
        }

        std::lock_guard<std::mutex> Lock(EventMutex);
        FPairKey                    Key(A, B);
        if (!Key.IsValid())
        {
            return;
        }

        if (bBegin)
        {
            if (ActiveHitPairs.find(Key) != ActiveHitPairs.end())
            {
                return;
            }
            ActiveHitPairs[Key] = FPairRecord { A, B };
        }
        else
        {
            auto It = ActiveHitPairs.find(Key);
            if (It == ActiveHitPairs.end())
            {
                return;
            }
            ActiveHitPairs.erase(It);
        }

        PendingHits.push_back({ A, B, NormalImpulse, WorldLocation, WorldNormal, PenetrationDepth, bBegin });
        PendingHits.push_back({ B, A, NormalImpulse * -1.0f, WorldLocation, WorldNormal * -1.0f, PenetrationDepth, bBegin });
    }

    void QueueSyntheticEndEventsForComponent_NoLock(
        UPrimitiveComponent*                                     Component,
        std::unordered_map<FPairKey, FPairRecord, FPairKeyHash>& ActivePairs,
        std::vector<FQueuedTrigger>&                             OutTriggers,
        bool
    )
    {
        for (auto It = ActivePairs.begin(); It != ActivePairs.end();)
        {
            UPrimitiveComponent* A = It->second.A.GetEvenIfPendingKill();
            UPrimitiveComponent* B = It->second.B.GetEvenIfPendingKill();
            if (A == Component || B == Component || !A || !B)
            {
                UPrimitiveComponent* Other = (A == Component) ? B : A;
                if (IsValid(Component) && IsValid(Other) && Component->GetGenerateOverlapEvents())
                {
                    OutTriggers.push_back({ Component, Other, false });
                }
                if (IsValid(Other) && IsValid(Component) && Other->GetGenerateOverlapEvents())
                {
                    OutTriggers.push_back({ Other, Component, false });
                }
                It = ActivePairs.erase(It);
            }
            else
            {
                ++It;
            }
        }
    }

    void QueueSyntheticEndEventsForComponent_NoLock(
        UPrimitiveComponent*                                     Component,
        std::unordered_map<FPairKey, FPairRecord, FPairKeyHash>& ActivePairs,
        std::vector<FQueuedHit>&                                 OutHits,
        bool
    )
    {
        for (auto It = ActivePairs.begin(); It != ActivePairs.end();)
        {
            UPrimitiveComponent* A = It->second.A.GetEvenIfPendingKill();
            UPrimitiveComponent* B = It->second.B.GetEvenIfPendingKill();
            if (A == Component || B == Component || !A || !B)
            {
                UPrimitiveComponent* Other = (A == Component) ? B : A;
                if (IsValid(Component) && IsValid(Other))
                {
                    OutHits.push_back({ Component, Other, FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector, 0.0f, false });
                }
                if (IsValid(Other) && IsValid(Component))
                {
                    OutHits.push_back({ Other, Component, FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector, 0.0f, false });
                }
                It = ActivePairs.erase(It);
            }
            else
            {
                ++It;
            }
        }
    }

    std::mutex                                              EventMutex;
    std::vector<FQueuedHit>                                 PendingHits;
    std::vector<FQueuedTrigger>                             PendingTriggers;
    std::unordered_map<FPairKey, FPairRecord, FPairKeyHash> ActiveOverlapPairs;
    std::unordered_map<FPairKey, FPairRecord, FPairKeyHash> ActiveHitPairs;
};

// ============================================================
// Collision Filtering
// ============================================================
namespace
{
    ECollisionResponse GetPackedResponseTo(const PxFilterData& From, PxU32 ToChannel)
    {
        const PxU32 ChannelBit = 1u << ToChannel;
        if ((From.word1 & ChannelBit) != 0)
        {
            return ECollisionResponse::Block;
        }
        if ((From.word2 & ChannelBit) != 0)
        {
            return ECollisionResponse::Overlap;
        }
        return ECollisionResponse::Ignore;
    }

    ECollisionResponse GetPackedMinResponse(const PxFilterData& A, const PxFilterData& B)
    {
        const PxU32              ChannelA = GetPhysicsFilterObjectType(A.word0);
        const PxU32              ChannelB = GetPhysicsFilterObjectType(B.word0);
        const ECollisionResponse AToB     = GetPackedResponseTo(A, ChannelB);
        const ECollisionResponse BToA     = GetPackedResponseTo(B, ChannelA);
        return (AToB < BToA) ? AToB : BToA;
    }

    bool IsPackedQueryOnly(const PxFilterData& Data)
    {
        return HasPhysicsFilterFlag(Data.word0, PhysicsFilter_QueryOnly);
    }

    bool IsPackedPhysicsEnabled(const PxFilterData& Data)
    {
        return HasPhysicsFilterFlag(Data.word0, PhysicsFilter_PhysicsOnly) ||
                HasPhysicsFilterFlag(Data.word0, PhysicsFilter_QueryAndPhysics);
    }

    bool IsPackedQueryEnabled(const PxFilterData& Data)
    {
        return HasPhysicsFilterFlag(Data.word0, PhysicsFilter_QueryOnly) ||
                HasPhysicsFilterFlag(Data.word0, PhysicsFilter_QueryAndPhysics);
    }

    bool WantsHitNotify(const PxFilterData& A, const PxFilterData& B)
    {
        return HasPhysicsFilterFlag(A.word0, PhysicsFilter_GenerateHitEvents) ||
                HasPhysicsFilterFlag(B.word0, PhysicsFilter_GenerateHitEvents);
    }

    bool WantsOverlapNotify(const PxFilterData& A, const PxFilterData& B)
    {
        return (IsPackedQueryEnabled(A) || IsPackedQueryEnabled(B)) &&
        (HasPhysicsFilterFlag(A.word0, PhysicsFilter_GenerateOverlapEvents) ||
            HasPhysicsFilterFlag(B.word0, PhysicsFilter_GenerateOverlapEvents));
    }
}

static PxFilterFlags KraftonFilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void*, PxU32)
{
    if (filterData0.word3 != 0 && filterData0.word3 == filterData1.word3)
    {
        return PxFilterFlag::eKILL;
    }

    const ECollisionResponse MinResponse = GetPackedMinResponse(filterData0, filterData1);
    if (MinResponse == ECollisionResponse::Ignore)
    {
        return PxFilterFlag::eKILL;
    }

    if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
    {
        if (!WantsOverlapNotify(filterData0, filterData1))
        {
            return PxFilterFlag::eKILL;
        }

        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;
    }

    if (MinResponse == ECollisionResponse::Block)
    {
        const bool bBothCanPhysicallySolve =
                IsPackedPhysicsEnabled(filterData0) && IsPackedPhysicsEnabled(filterData1) &&
                !IsPackedQueryOnly(filterData0) && !IsPackedQueryOnly(filterData1);

        pairFlags = bBothCanPhysicallySolve
                ? PxPairFlag::eCONTACT_DEFAULT
                : PxPairFlag::eDETECT_DISCRETE_CONTACT;

        if (WantsHitNotify(filterData0, filterData1))
        {
            pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND
                    | PxPairFlag::eNOTIFY_TOUCH_LOST
                    | PxPairFlag::eNOTIFY_CONTACT_POINTS;
        }

        return PxFilterFlag::eDEFAULT;
    }

    if (MinResponse == ECollisionResponse::Overlap)
    {
        if (!WantsOverlapNotify(filterData0, filterData1))
        {
            return PxFilterFlag::eKILL;
        }

        pairFlags = PxPairFlag::eDETECT_DISCRETE_CONTACT
            | PxPairFlag::eNOTIFY_TOUCH_FOUND
            | PxPairFlag::eNOTIFY_TOUCH_LOST;
        return PxFilterFlag::eDEFAULT;
    }

    return PxFilterFlag::eKILL;
}

void FPhysXPhysicsScene::Initialize(UWorld* InWorld)
{
    World = InWorld;

    AcquireSharedPhysX(Foundation, Physics);
    if (!Foundation || !Physics)
    {
        UE_LOG("[PhysX] Failed to create Foundation or Physics");
        return;
    }

    const auto& PhysicsSettings   = FProjectSettings::Get().Physics;
    int32       WorkerThreadCount = PhysicsSettings.WorkerThreadCount;
    if (WorkerThreadCount <= 0)
    {
        const unsigned int HardwareThreads = std::thread::hardware_concurrency();
        WorkerThreadCount                  = static_cast<int32>((std::max)(1u, HardwareThreads > 1 ? HardwareThreads - 1 : 1u));
    }

    Dispatcher    = PxDefaultCpuDispatcherCreate(WorkerThreadCount);
    EventCallback = new FPhysXSimulationCallback();

    PxSceneDesc SceneDesc(Physics->getTolerancesScale());
    SceneDesc.gravity = PxVec3(0.0f, 0.0f, -9.81f);
    SceneDesc.cpuDispatcher = Dispatcher;
    SceneDesc.filterShader = KraftonFilterShader;
    SceneDesc.simulationEventCallback = EventCallback;
    SceneDesc.staticKineFilteringMode = PxPairFilteringMode::eKEEP;
    SceneDesc.kineKineFilteringMode = PxPairFilteringMode::eKEEP;

    if (PhysicsSettings.bEnableCCD)
    {
        SceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
    }
    if (PhysicsSettings.bEnablePCM)
    {
        SceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
    }
    if (PhysicsSettings.bEnableActiveActors)
    {
        SceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    }
    if (PhysicsSettings.bRequireSceneReadWriteLock)
    {
        SceneDesc.flags |= PxSceneFlag::eREQUIRE_RW_LOCK;
    }

    Scene = Physics->createScene(SceneDesc);
    if (!Scene)
    {
        UE_LOG("[PhysX] Failed to create Scene");
        return;
    }

    DefaultMaterial = Physics->createMaterial(0.5f, 0.5f, 0.3f);
    if (!DefaultMaterial)
    {
        UE_LOG("[PhysX] Failed to create DefaultMaterial");
        return;
    }

    Runtime.Initialize(World, Physics, Scene, DefaultMaterial);

    UE_LOG("[PhysX] Initialized successfully (Scene=%p)", Scene);
}

void FPhysXPhysicsScene::Shutdown()
{
    Runtime.Shutdown();

    if (DefaultMaterial)
    {
        DefaultMaterial->release();
        DefaultMaterial = nullptr;
    }
    if (Scene)
    {
        Scene->release();
        Scene = nullptr;
    }
    if (EventCallback)
    {
        delete EventCallback;
        EventCallback = nullptr;
    }
    if (Dispatcher)
    {
        Dispatcher->release();
        Dispatcher = nullptr;
    }

    Foundation = nullptr;
    Physics = nullptr;
    ReleaseSharedPhysX();

    World = nullptr;
}

void FPhysXPhysicsScene::RegisterComponent(UPrimitiveComponent* Comp)
{
    Runtime.RegisterComponent(Comp);
}

void FPhysXPhysicsScene::UnregisterComponent(UPrimitiveComponent* Comp)
{
    if (EventCallback)
    {
        EventCallback->NotifyComponentUnregistered(Comp);
    }
    Runtime.UnregisterComponent(Comp);
}

void FPhysXPhysicsScene::RebuildBody(UPrimitiveComponent* Comp)
{
    if (EventCallback)
    {
        EventCallback->NotifyComponentUnregistered(Comp);
    }
    Runtime.RebuildBody(Comp);
}

void FPhysXPhysicsScene::Tick(float DeltaTime)
{
    Runtime.Tick(DeltaTime);
}

void FPhysXPhysicsScene::DispatchPendingEvents()
{
    if (EventCallback)
    {
        const auto& PhysicsSettings = FProjectSettings::Get().Physics;
        EventCallback->DispatchPendingEvents(
            PhysicsSettings.bDispatchCollisionEvents,
            PhysicsSettings.bDispatchTriggerEvents
        );
    }
}

void FPhysXPhysicsScene::AddForce(UPrimitiveComponent* Comp, const FVector& Force)
{
    const FPhysicsBodyHandle Body = Runtime.FindBodyByComponent(Comp);
    if (Body.IsValid())
    {
        Runtime.AddForce(Body, Force);
    }
}

void FPhysXPhysicsScene::AddForceAtLocation(
    UPrimitiveComponent* Comp,
    const FVector& Force,
    const FVector& WorldLocation
)
{
    const FPhysicsBodyHandle Body = Runtime.FindBodyByComponent(Comp);
    if (Body.IsValid())
    {
        Runtime.AddForceAtLocation(Body, Force, WorldLocation);
    }
}

void FPhysXPhysicsScene::AddTorque(UPrimitiveComponent* Comp, const FVector& Torque)
{
    const FPhysicsBodyHandle Body = Runtime.FindBodyByComponent(Comp);
    if (Body.IsValid())
    {
        Runtime.AddTorque(Body, Torque);
    }
}

FVector FPhysXPhysicsScene::GetLinearVelocity(UPrimitiveComponent* Comp) const
{
    const FPhysicsBodyHandle Body = Runtime.FindBodyByComponent(Comp);
    return Body.IsValid() ? Runtime.GetBodyLinearVelocity(Body) : FVector::ZeroVector;
}

void FPhysXPhysicsScene::SetLinearVelocity(UPrimitiveComponent* Comp, const FVector& Vel)
{
    const FPhysicsBodyHandle Body = Runtime.FindBodyByComponent(Comp);
    if (Body.IsValid())
    {
        Runtime.SetBodyVelocity(Body, Vel, Runtime.GetBodyAngularVelocity(Body));
    }
}

FVector FPhysXPhysicsScene::GetAngularVelocity(UPrimitiveComponent* Comp) const
{
    const FPhysicsBodyHandle Body = Runtime.FindBodyByComponent(Comp);
    return Body.IsValid() ? Runtime.GetBodyAngularVelocity(Body) : FVector::ZeroVector;
}

void FPhysXPhysicsScene::SetAngularVelocity(UPrimitiveComponent* Comp, const FVector& Vel)
{
    const FPhysicsBodyHandle Body = Runtime.FindBodyByComponent(Comp);
    if (Body.IsValid())
    {
        Runtime.SetBodyVelocity(Body, Runtime.GetBodyLinearVelocity(Body), Vel);
    }
}

void FPhysXPhysicsScene::SetMass(UPrimitiveComponent* Comp, float Mass)
{
    const FPhysicsBodyHandle Body = Runtime.FindBodyByComponent(Comp);
    if (Body.IsValid())
    {
        Runtime.SetMass(Body, Mass);
    }
}

float FPhysXPhysicsScene::GetMass(UPrimitiveComponent* Comp) const
{
    const FPhysicsBodyHandle Body = Runtime.FindBodyByComponent(Comp);
    return Body.IsValid() ? Runtime.GetMass(Body) : 1.0f;
}

void FPhysXPhysicsScene::SetCenterOfMass(UPrimitiveComponent* Comp, const FVector& LocalOffset)
{
    const FPhysicsBodyHandle Body = Runtime.FindBodyByComponent(Comp);
    if (Body.IsValid())
    {
        Runtime.SetCenterOfMass(Body, LocalOffset);
    }
}

FVector FPhysXPhysicsScene::GetCenterOfMass(UPrimitiveComponent* Comp) const
{
    const FPhysicsBodyHandle Body = Runtime.FindBodyByComponent(Comp);
    return Body.IsValid() ? Runtime.GetCenterOfMass(Body) : FVector::ZeroVector;
}

void FPhysXPhysicsScene::SetLinearLock(UPrimitiveComponent* Comp, bool bX, bool bY, bool bZ)
{
    const FPhysicsBodyHandle Body = Runtime.FindBodyByComponent(Comp);
    if (Body.IsValid())
    {
        Runtime.SetLinearLock(Body, bX, bY, bZ);
    }
}

void FPhysXPhysicsScene::SetAngularLock(UPrimitiveComponent* Comp, bool bX, bool bY, bool bZ)
{
    const FPhysicsBodyHandle Body = Runtime.FindBodyByComponent(Comp);
    if (Body.IsValid())
    {
        Runtime.SetAngularLock(Body, bX, bY, bZ);
    }
}

bool FPhysXPhysicsScene::Raycast(
    const FVector& Start,
    const FVector& Dir,
    float MaxDist,
    FHitResult& OutHit,
    ECollisionChannel TraceChannel,
    const AActor* IgnoreActor
) const
{
    OutHit = FHitResult();
    if (!Scene || MaxDist <= 0.0f)
    {
        return false;
    }

    FVector RayDir = Dir;
    if (RayDir.IsNearlyZero())
    {
        return false;
    }
    RayDir.Normalize();

    struct FChannelRaycastFilter : PxQueryFilterCallback
    {
        const AActor* IgnoreActor = nullptr;
        PxU32 TraceBit = 0;

        FChannelRaycastFilter(const AActor* InIgnoreActor, ECollisionChannel InChannel)
            : IgnoreActor(InIgnoreActor)
            , TraceBit(1u << static_cast<PxU32>(InChannel))
        {
        }

        PxQueryHitType::Enum preFilter(
            const PxFilterData&,
            const PxShape* Shape,
            const PxRigidActor* Actor,
            PxHitFlags&
        ) override
        {
            AActor* ShapeOwner = GetOwnerActorFromShape(Shape);
            AActor* ActorOwner = GetOwnerActorFromActor(Actor);
            if (IgnoreActor && (ShapeOwner == IgnoreActor || ActorOwner == IgnoreActor))
            {
                return PxQueryHitType::eNONE;
            }

            if (Shape)
            {
                if (Shape->getFlags().isSet(PxShapeFlag::eTRIGGER_SHAPE))
                {
                    return PxQueryHitType::eNONE;
                }

                const PxFilterData ShapeData = Shape->getQueryFilterData();
                if ((ShapeData.word1 & TraceBit) == 0)
                {
                    return PxQueryHitType::eNONE;
                }
            }

            return PxQueryHitType::eBLOCK;
        }

        PxQueryHitType::Enum postFilter(const PxFilterData&, const PxQueryHit&) override
        {
            return PxQueryHitType::eBLOCK;
        }
    };

    PxRaycastBuffer Hit;
    PxQueryFilterData FilterData;
    FilterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;
    FChannelRaycastFilter FilterCallback(IgnoreActor, TraceChannel);

    PxSceneReadLock ReadLock(*Scene);

    const bool bStatus = Scene->raycast(
        ToPxVec3(Start),
        ToPxVec3(RayDir),
        MaxDist,
        Hit,
        PxHitFlag::eDEFAULT,
        FilterData,
        &FilterCallback
    );

    if (!bStatus || !Hit.hasBlock)
    {
        return false;
    }

    const PxRaycastHit& Block = Hit.block;
    OutHit.bHit = true;
    OutHit.Distance = Block.distance;
    OutHit.WorldHitLocation = ToFVector(Block.position);
    OutHit.ImpactNormal = ToFVector(Block.normal);
    OutHit.WorldNormal = OutHit.ImpactNormal;

    if (Block.shape)
    {
        UPrimitiveComponent* HitComp = GetComponentFromShape(Block.shape);
        if (IsValid(HitComp))
        {
            OutHit.HitComponent = HitComp;
            OutHit.HitActor = HitComp->GetOwner();
        }
    }

    if (!OutHit.HitActor && Block.actor)
    {
        AActor* HitActor = GetOwnerActorFromActor(Block.actor);
        if (IsValid(HitActor))
        {
            OutHit.HitActor = HitActor;
        }
    }

    if (!OutHit.HitComponent && !OutHit.HitActor)
    {
        OutHit.bHit = false;
        return false;
    }

    return true;
}

bool FPhysXPhysicsScene::RaycastByObjectTypes(
    const FVector& Start,
    const FVector& Dir,
    float MaxDist,
    FHitResult& OutHit,
    uint32 ObjectTypeMask,
    const AActor* IgnoreActor
) const
{
    OutHit = FHitResult();
    if (!Scene || ObjectTypeMask == 0 || MaxDist <= 0.0f)
    {
        return false;
    }

    FVector RayDir = Dir;
    if (RayDir.IsNearlyZero())
    {
        return false;
    }
    RayDir.Normalize();

    struct FObjectTypeRaycastFilter : PxQueryFilterCallback
    {
        const AActor* IgnoreActor = nullptr;
        PxU32 ObjectTypeMask = 0;

        FObjectTypeRaycastFilter(const AActor* InIgnoreActor, PxU32 InMask)
            : IgnoreActor(InIgnoreActor)
            , ObjectTypeMask(InMask)
        {
        }

        PxQueryHitType::Enum preFilter(
            const PxFilterData&,
            const PxShape* Shape,
            const PxRigidActor* Actor,
            PxHitFlags&
        ) override
        {
            AActor* ShapeOwner = GetOwnerActorFromShape(Shape);
            AActor* ActorOwner = GetOwnerActorFromActor(Actor);
            if (IgnoreActor && (ShapeOwner == IgnoreActor || ActorOwner == IgnoreActor))
            {
                return PxQueryHitType::eNONE;
            }

            if (Shape)
            {
                if (Shape->getFlags().isSet(PxShapeFlag::eTRIGGER_SHAPE))
                {
                    return PxQueryHitType::eNONE;
                }

                const PxFilterData ShapeData      = Shape->getQueryFilterData();
                const PxU32        ShapeObjectBit = 1u << GetPhysicsFilterObjectType(ShapeData.word0);
                if ((ShapeObjectBit & ObjectTypeMask) == 0)
                {
                    return PxQueryHitType::eNONE;
                }
            }

            return PxQueryHitType::eBLOCK;
        }

        PxQueryHitType::Enum postFilter(const PxFilterData&, const PxQueryHit&) override
        {
            return PxQueryHitType::eBLOCK;
        }
    };

    PxRaycastBuffer Hit;
    PxQueryFilterData FilterData;
    FilterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;
    FObjectTypeRaycastFilter FilterCallback(IgnoreActor, ObjectTypeMask);

    PxSceneReadLock ReadLock(*Scene);

    const bool bStatus = Scene->raycast(
        ToPxVec3(Start),
        ToPxVec3(RayDir),
        MaxDist,
        Hit,
        PxHitFlag::eDEFAULT,
        FilterData,
        &FilterCallback
    );

    if (!bStatus || !Hit.hasBlock)
    {
        return false;
    }

    const PxRaycastHit& Block = Hit.block;
    OutHit.bHit = true;
    OutHit.Distance = Block.distance;
    OutHit.WorldHitLocation = ToFVector(Block.position);
    OutHit.ImpactNormal = ToFVector(Block.normal);
    OutHit.WorldNormal = OutHit.ImpactNormal;

    if (Block.shape)
    {
        UPrimitiveComponent* HitComp = GetComponentFromShape(Block.shape);
        if (IsValid(HitComp))
        {
            OutHit.HitComponent = HitComp;
            OutHit.HitActor = HitComp->GetOwner();
        }
    }

    if (!OutHit.HitActor && Block.actor)
    {
        AActor* HitActor = GetOwnerActorFromActor(Block.actor);
        if (IsValid(HitActor))
        {
            OutHit.HitActor = HitActor;
        }
    }

    if (!OutHit.HitComponent && !OutHit.HitActor)
    {
        OutHit.bHit = false;
        return false;
    }

    return true;
}
