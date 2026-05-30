#include "Physics/PhysXPhysicsRuntime.h"
#include "Physics/PhysXBodyBuilder.h"
#include "Physics/PhysXConstraintBuilder.h"
#include "Physics/PhysXConversion.h"

#include "Component/PrimitiveComponent.h"
#include "Core/ProjectSettings.h"
#include "Component/Shape/BoxComponent.h"
#include "Component/Shape/CapsuleComponent.h"
#include "Component/Shape/SphereComponent.h"
#include "Core/Logging/Log.h"
#include "Core/Types/EngineTypes.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"
#include "Object/Object.h"

#include <algorithm>
#include <chrono>
#include <PxPhysicsAPI.h>

using namespace physx;

namespace
{
    bool IsHandleIndexValid(uint32 Index, size_t Size)
    {
        return Index != UINT32_MAX && static_cast<size_t>(Index) < Size;
    }

    uint32 GetObjectKey(const UObject* Object)
    {
        return IsAliveObject(Object) ? Object->GetUUID() : 0;
    }

    FTransform ComposePhysicsTransforms(const FTransform& ParentWorld, const FTransform& Local)
    {
        FTransform Result = Local;
        Result.Location =
            ParentWorld.Location + ParentWorld.Rotation.RotateVector(Local.Location);
        Result.Rotation = ParentWorld.Rotation * Local.Rotation;
        return Result;
    }

    FTransform GetComponentWorldTransform(UPrimitiveComponent* Comp)
    {
        FTransform Result;
        if (!Comp)
        {
            return Result;
        }

        Result.Location = Comp->GetWorldLocation();
        Result.Rotation = Comp->GetWorldMatrix().ToQuat();
        Result.Scale = FVector::OneVector;
        return Result;
    }

    FTransform MakeRelativeTransformFromWorld(
        const FVector& ChildWorldLocation,
        const FQuat& ChildWorldRotation,
        UPrimitiveComponent* Root
    )
    {
        FTransform Result;

        if (!Root)
        {
            Result.Location = ChildWorldLocation;
            Result.Rotation = ChildWorldRotation;
            Result.Scale = FVector::OneVector;
            return Result;
        }

        const FVector RootPos = Root->GetWorldLocation();
        const FQuat RootRot = Root->GetWorldMatrix().ToQuat();
        const FQuat InvRootRot = RootRot.Inverse();

        Result.Location = InvRootRot.RotateVector(ChildWorldLocation - RootPos);
        Result.Rotation = InvRootRot * ChildWorldRotation;
        Result.Scale = FVector::OneVector;
        return Result;
    }

    FTransform MakeShapeLocalTransform(
        UPrimitiveComponent* Comp,
        UPrimitiveComponent* Root
    )
    {
        if (!Comp || !Root || Comp == Root)
        {
            return FTransform();
        }

        return MakeRelativeTransformFromWorld(
            Comp->GetWorldLocation(),
            Comp->GetWorldMatrix().ToQuat(),
            Root
        );
    }

    bool HasAnyBlockResponse(UPrimitiveComponent* Comp)
    {
        if (!Comp)
        {
            return false;
        }

        for (int32 Ch = 0; Ch < static_cast<int32>(ECollisionChannel::ActiveCount); ++Ch)
        {
            if (Comp->GetCollisionResponseToChannel(static_cast<ECollisionChannel>(Ch))
                == ECollisionResponse::Block)
            {
                return true;
            }
        }
        return false;
    }

    bool HasAnyOverlapResponse(UPrimitiveComponent* Comp)
    {
        if (!Comp)
        {
            return false;
        }

        for (int32 Ch = 0; Ch < static_cast<int32>(ECollisionChannel::ActiveCount); ++Ch)
        {
            if (Comp->GetCollisionResponseToChannel(static_cast<ECollisionChannel>(Ch))
                == ECollisionResponse::Overlap)
            {
                return true;
            }
        }
        return false;
    }

    bool ShouldBeTriggerShape(UPrimitiveComponent* Comp, bool bOwnerBodyIsDynamic)
    {
        if (!Comp || Comp->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
        {
            return false;
        }

        const bool bHasAnyBlock     = HasAnyBlockResponse(Comp);
        bool       bShouldBeTrigger =
                (Comp->GetCollisionObjectType() == ECollisionChannel::Trigger && !bHasAnyBlock) ||
                (Comp->GetCollisionEnabled() == ECollisionEnabled::QueryOnly &&
                    !bHasAnyBlock &&
                    HasAnyOverlapResponse(Comp));

        if (bShouldBeTrigger && bOwnerBodyIsDynamic)
        {
            bShouldBeTrigger = false;
        }

        return bShouldBeTrigger;
    }

    void FillFilterDataFromComponent(
        FPhysicsFilterData&  Out,
        UPrimitiveComponent* Comp,
        bool                 bIsTriggerShape
    )
    {
        if (!Comp)
        {
            return;
        }

        Out.ObjectType             = static_cast<uint32>(Comp->GetCollisionObjectType());
        Out.BlockMask              = 0;
        Out.OverlapMask            = 0;
        Out.IgnoreGroup            = Comp->GetOwner() ? Comp->GetOwner()->GetUUID() : 0;
        Out.CollisionEnabled       = Comp->GetCollisionEnabled();
        Out.bIsTrigger             = bIsTriggerShape;
        Out.bGenerateHitEvents     = true;
        Out.bGenerateOverlapEvents = Comp->GetGenerateOverlapEvents();

        for (int32 Ch = 0; Ch < static_cast<int32>(ECollisionChannel::ActiveCount); ++Ch)
        {
            const ECollisionResponse R =
                Comp->GetCollisionResponseToChannel(static_cast<ECollisionChannel>(Ch));

            if (R == ECollisionResponse::Block)
            {
                Out.BlockMask |= (1u << Ch);
            }
            else if (R == ECollisionResponse::Overlap)
            {
                Out.OverlapMask |= (1u << Ch);
            }
        }
    }
}

void FPhysXPhysicsRuntime::Initialize(
    UWorld* InWorld,
    PxPhysics* InPhysics,
    PxScene* InScene,
    PxMaterial* InDefaultMaterial
)
{
    World = InWorld;
    Physics = InPhysics;
    Scene = InScene;
    DefaultMaterial = InDefaultMaterial;

    const auto& PhysicsSettings = FProjectSettings::Get().Physics;
    FixedDt                     = (std::max)(1.0f / 240.0f, PhysicsSettings.FixedTimeStep);
    MaxFrameDt                  = (std::max)(FixedDt, PhysicsSettings.MaxFrameDeltaTime);
    MaxSubsteps                 = (std::max)(1, PhysicsSettings.MaxSubsteps);
    bDebugSnapshotEnabled       = PhysicsSettings.bBuildDebugSnapshot;

    Accumulator = 0.0f;
    StepIndex   = 0;
    Stats       = FPhysicsStats();
}

void FPhysXPhysicsRuntime::Shutdown()
{
    const bool bSceneLockedForShutdown = Scene != nullptr;
    if (bSceneLockedForShutdown)
    {
        Scene->lockWrite();
    }

    for (auto& ConstraintPtr : Constraints)
    {
        if (ConstraintPtr && ConstraintPtr->Joint)
        {
            ConstraintPtr->Joint->release();
            ConstraintPtr->Joint = nullptr;
        }
    }

    for (auto& BodyPtr : Bodies)
    {
        if (BodyPtr && BodyPtr->Actor)
        {
            if (Scene && BodyPtr->bRegisteredInScene)
            {
                Scene->removeActor(*BodyPtr->Actor);
            }

            BodyPtr->Actor->release();
            BodyPtr->Actor = nullptr;
        }
    }

    Constraints.clear();
    ConstraintGenerations.clear();

    Shapes.clear();
    ShapeGenerations.clear();

    Bodies.clear();
    BodyGenerations.clear();

    ActorCompounds.clear();
    ComponentToBody.clear();
    ComponentToShape.clear();

    CommandQueue.Clear();
    Stats = FPhysicsStats();
    Accumulator = 0.0f;

    if (bSceneLockedForShutdown)
    {
        Scene->unlockWrite();
    }

    World = nullptr;
    Physics = nullptr;
    Scene = nullptr;
    DefaultMaterial = nullptr;
}

void FPhysXPhysicsRuntime::Tick(float DeltaTime)
{
    if (!Scene || DeltaTime <= 0.0f)
    {
        Stats.NumSubsteps = 0;
        if (Scene)
        {
            PxSceneReadLock ReadLock(*Scene);
            UpdateStats();
        }
        else
        {
            UpdateStats();
        }
        return;
    }

    if (DeltaTime > MaxFrameDt)
    {
        DeltaTime = MaxFrameDt;
    }

    Accumulator += DeltaTime;

    int32 StepCount = 0;

    // 프레임 누적 타이밍 (substep 합산). UpdateStats() 가 Stats 를 전부 초기화하므로,
    // 측정값은 local 에 모아 두었다가 UpdateStats() 호출 이후에 Stats 로 복사한다.
    using FClock = std::chrono::high_resolution_clock;
    const auto DurationMs = [](FClock::time_point A, FClock::time_point B)
    {
        return std::chrono::duration<float, std::milli>(B - A).count();
    };
    float PrePhysicsMs           = 0.0f;
    float ApplyCommandsMs        = 0.0f;
    float SyncEngineToPhysicsMs  = 0.0f;
    float SimulateMs             = 0.0f;
    float FetchResultsMs         = 0.0f;
    float SyncPhysicsToEngineMs  = 0.0f;
    float PostPhysicsMs          = 0.0f;
    int32 AppliedCommands        = 0;
    int32 PendingCommandsAtDrain = 0;

    while (Accumulator >= FixedDt && StepCount < MaxSubsteps)
    {
        const auto T0 = FClock::now();

        auto TApplyEnd = T0;
        {
            PxSceneWriteLock WriteLock(*Scene);
            PendingCommandsAtDrain += CommandQueue.Num();
            AppliedCommands        += ApplyPendingCommands();
            TApplyEnd              = FClock::now();

            for (auto& BodyPtr : Bodies)
            {
                if (!BodyPtr || !BodyPtr->Actor)
                {
                    continue;
                }

                FBodyInstance& Body = *BodyPtr;
                if (Body.ShouldPushTransformToPhysics() || Body.IsKinematicTargetDriven())
                {
                    SyncEngineToPhysics(Body);
                }
            }
        }

        const auto T1 = FClock::now();
        auto       T2 = T1;
        auto       T3 = T1;
        {
            PxSceneWriteLock WriteLock(*Scene);
            Scene->simulate(FixedDt);
            T2 = FClock::now();
            Scene->fetchResults(true);
            T3 = FClock::now();
        }

        {
            PxSceneReadLock ReadLock(*Scene);
            for (auto& BodyPtr : Bodies)
            {
                if (!BodyPtr || !BodyPtr->Actor)
                {
                    continue;
                }

                FBodyInstance& Body = *BodyPtr;
                if (Body.ShouldPullTransformToEngine())
                {
                    SyncPhysicsToEngine(Body);
                }
                else
                {
                    Body.PreviousTransform = Body.CurrentTransform;
                    Body.CurrentTransform  = ToFTransform(Body.Actor->getGlobalPose());
                    if (PxRigidDynamic* Dynamic = Body.Actor->is<PxRigidDynamic>())
                    {
                        Body.LinearVelocity  = ToFVector(Dynamic->getLinearVelocity());
                        Body.AngularVelocity = ToFVector(Dynamic->getAngularVelocity());
                        Body.bIsSleeping     = Dynamic->isSleeping();
                    }
                }
            }
        }

        const auto T4 = FClock::now();

        ApplyCommandsMs       += DurationMs(T0, TApplyEnd);
        SyncEngineToPhysicsMs += DurationMs(TApplyEnd, T1);
        PrePhysicsMs          += DurationMs(T0, T1);
        SimulateMs            += DurationMs(T1, T2);
        FetchResultsMs        += DurationMs(T2, T3);
        SyncPhysicsToEngineMs += DurationMs(T3, T4);
        PostPhysicsMs         += DurationMs(T3, T4);

        Accumulator -= FixedDt;
        ++StepCount;
        ++StepIndex;
    }

    int32 DroppedSubsteps = 0;
    if (StepCount == MaxSubsteps)
    {
        // 남은 누적 시간을 버린다 (spiral-of-death 방지). 버린 step 수를 기록.
        DroppedSubsteps = (FixedDt > 0.0f) ? static_cast<int32>(Accumulator / FixedDt) : 0;
        Accumulator     = 0.0f;
    }

    Stats.NumSubsteps = StepCount;
    {
        PxSceneReadLock ReadLock(*Scene);
        UpdateStats();
    }

    // UpdateStats() 가 Stats 를 초기화한 뒤이므로 여기서 타이밍/누적 상태를 채운다.
    Stats.PrePhysicsMs          = PrePhysicsMs;
    Stats.ApplyCommandsMs       = ApplyCommandsMs;
    Stats.SyncEngineToPhysicsMs = SyncEngineToPhysicsMs;
    Stats.SimulateMs            = SimulateMs;
    Stats.FetchResultsMs        = FetchResultsMs;
    Stats.SyncPhysicsToEngineMs = SyncPhysicsToEngineMs;
    Stats.PostPhysicsMs         = PostPhysicsMs;
    Stats.NumDroppedSubsteps    = DroppedSubsteps;
    Stats.AccumulatorSeconds    = Accumulator;
    Stats.InterpolationAlpha    = (FixedDt > 0.0f) ? (Accumulator / FixedDt) : 0.0f;
    Stats.NumPendingCommands    = PendingCommandsAtDrain;
    Stats.NumAppliedCommands    = AppliedCommands;

    // step 직후 한 곳에서만 live PhysX 를 읽어 Debug/Stat 스냅샷을 publish 한다.
    if (bDebugSnapshotEnabled)
    {
        const auto SnapStart = FClock::now();
        {
            PxSceneReadLock ReadLock(*Scene);
            BuildDebugSnapshot_Internal();
        }
        Stats.BuildSnapshotMs = DurationMs(SnapStart, FClock::now());
        {
            std::lock_guard<std::mutex> Lock(DebugSnapshotMutex);
            DebugSnapshot.Stats = Stats;
        }
    }
}


void FPhysXPhysicsRuntime::RegisterComponent(UPrimitiveComponent* Comp)
{
    if (!IsAliveObject(Comp) || !Scene)
    {
        return;
    }

    // Registration allocates the body/shape handle immediately so same-frame
    // gameplay calls such as AddForce can resolve the component. Destructive
    // mutation paths (unregister/rebuild/destroy) remain command-boundary based.
    PxSceneWriteLock WriteLock(*Scene);
    RegisterComponent_Internal(Comp);
}

void FPhysXPhysicsRuntime::UnregisterComponent(UPrimitiveComponent* Comp)
{
    if (!Comp)
    {
        return;
    }

    FPhysicsCommand Cmd;
    Cmd.Type      = EPhysicsCommandType::UnregisterComponent;
    Cmd.Component = Comp;
    EnqueueCommand(Cmd);
}

void FPhysXPhysicsRuntime::RebuildBody(UPrimitiveComponent* Comp)
{
    if (!IsAliveObject(Comp))
    {
        return;
    }

    FPhysicsCommand Cmd;
    Cmd.Type      = EPhysicsCommandType::RebuildComponentBody;
    Cmd.Component = Comp;
    EnqueueCommand(Cmd);
}

void FPhysXPhysicsRuntime::RegisterComponent_Internal(UPrimitiveComponent* Comp)
{
    if (!IsValid(Comp) || !Physics || !Scene || !DefaultMaterial)
    {
        return;
    }

    if (ComponentToShape.find(GetObjectKey(Comp)) != ComponentToShape.end())
    {
        return;
    }

    AActor* OwnerActor = Comp->GetOwner();
    if (!IsValid(OwnerActor))
    {
        return;
    }

    FActorCompoundBody* Compound = FindCompoundByActor(OwnerActor);

    if (!Compound)
    {
        UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent());
        if (!RootPrim)
        {
            RootPrim = Comp;
        }

        FBodyCreationDesc BodyDesc = BuildBodyDescFromComponent(RootPrim);
        BodyDesc.Shapes.clear();

        FPhysicsBodyHandle BodyHandle = CreateRigidBody_Internal(BodyDesc);
        if (!BodyHandle.IsValid())
        {
            return;
        }

        FActorCompoundBody NewCompound;
        NewCompound.OwnerActor = OwnerActor;
        NewCompound.RootComponent = RootPrim;
        NewCompound.Body = BodyHandle;

        ActorCompounds[GetObjectKey(OwnerActor)] = NewCompound;
        Compound                                 = &ActorCompounds[GetObjectKey(OwnerActor)];
    }

    FPhysicsShapeDesc   ShapeDesc   = BuildShapeDescFromComponent(Comp, Compound->RootComponent.Get());
    FPhysicsShapeHandle ShapeHandle = AddShapeToBody(Compound->Body, Comp, ShapeDesc);
    if (!ShapeHandle.IsValid())
    {
        return;
    }

    Compound->Components.push_back(Comp);
    ComponentToBody[GetObjectKey(Comp)]  = Compound->Body;
    ComponentToShape[GetObjectKey(Comp)] = ShapeHandle;

    FBodyInstance* Body = ResolveBody(Compound->Body);
    if (Body && Body->Actor)
    {
        FBodyCreationDesc MassDesc = BuildBodyDescFromComponent(Compound->RootComponent.Get());
        FPhysXBodyBuilder::UpdateMassAndInertia(Body->Actor, MassDesc);
    }
}

void FPhysXPhysicsRuntime::UnregisterComponent_Internal(UPrimitiveComponent* Comp)
{
    if (!Comp)
    {
        return;
    }

    auto BodyIt = ComponentToBody.find(GetObjectKey(Comp));
    if (BodyIt == ComponentToBody.end())
    {
        return;
    }

    const FPhysicsBodyHandle BodyHandle = BodyIt->second;
    AActor* OwnerActor = Comp->GetOwner();

    if (!Scene)
    {
        return;
    }

    DetachShapeForComponent(Comp);
    ComponentToBody.erase(GetObjectKey(Comp));

    FActorCompoundBody* Compound = FindCompoundByActor(OwnerActor);
    if (!Compound)
    {
        // Owner가 이미 PendingKill/Garbage인 cleanup 경로에서는 actor compound map을
        // 찾지 못할 수 있다. 이 경우 component가 가리키던 body 전체를 안전하게 제거한다.
        DestroyRigidBody_Internal(BodyHandle);
        return;
    }

    Compound->Components.erase(
        std::remove_if(
            Compound->Components.begin(),
            Compound->Components.end(),
            [Comp](const TWeakObjectPtr<UPrimitiveComponent>& WeakComponent)
            {
                UPrimitiveComponent* C = WeakComponent.GetEvenIfPendingKill();
                return C == nullptr || C == Comp;
            }
        ),
        Compound->Components.end()
    );

    if (Compound->Components.empty())
    {
        DestroyRigidBody_Internal(BodyHandle);
        ActorCompounds.erase(GetObjectKey(OwnerActor));
        return;
    }

    FBodyInstance* Body = ResolveBody(Compound->Body);
    if (Body && Body->Actor)
    {
        FBodyCreationDesc MassDesc = BuildBodyDescFromComponent(Compound->RootComponent.Get());
        FPhysXBodyBuilder::UpdateMassAndInertia(Body->Actor, MassDesc);
    }
}

void FPhysXPhysicsRuntime::RebuildBody_Internal(UPrimitiveComponent* Comp)
{
    if (!IsValid(Comp))
    {
        return;
    }

    AActor* OwnerActor = Comp->GetOwner();
    if (!IsValid(OwnerActor))
    {
        return;
    }

    FActorCompoundBody* Compound = FindCompoundByActor(OwnerActor);
    if (!Compound)
    {
        return;
    }

    TArray<UPrimitiveComponent*> Components;
    for (const TWeakObjectPtr<UPrimitiveComponent>& WeakComponent : Compound->Components)
    {
        if (UPrimitiveComponent* C = WeakComponent.Get())
        {
            Components.push_back(C);
        }
    }

    for (UPrimitiveComponent* C : Components)
    {
        if (IsValid(C))
        {
            UnregisterComponent_Internal(C);
        }
    }

    for (UPrimitiveComponent* C : Components)
    {
        if (IsValid(C) && C->IsCollisionEnabled())
        {
            RegisterComponent_Internal(C);
        }
    }
}

FPhysicsBodyHandle FPhysXPhysicsRuntime::FindBodyByComponent(UPrimitiveComponent* Comp) const
{
    auto It = ComponentToBody.find(GetObjectKey(Comp));
    return It != ComponentToBody.end() ? It->second : FPhysicsBodyHandle{};
}

FPhysicsShapeHandle FPhysXPhysicsRuntime::FindShapeByComponent(UPrimitiveComponent* Comp) const
{
    auto It = ComponentToShape.find(GetObjectKey(Comp));
    return It != ComponentToShape.end() ? It->second : FPhysicsShapeHandle{};
}

FPhysicsBodyHandle FPhysXPhysicsRuntime::CreateRigidBody_Internal(const FBodyCreationDesc& Desc)
{
    if (!Physics || !Scene)
    {
        return {};
    }

    FPhysicsBodyHandle BodyHandle = AllocateBody();
    FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body)
    {
        return {};
    }

    PxRigidActor* Actor = FPhysXBodyBuilder::CreateRigidActor(Physics, Desc);
    if (!Actor)
    {
        FreeBody(BodyHandle);
        return {};
    }

    Body->OwnerActor = Desc.OwnerActor;
    Body->OwnerComponent = Desc.OwnerComponent;
    Body->BoneName = Desc.BoneName;
    Body->Actor = Actor;
    Body->BodyType = Desc.BodyType;
    Body->SyncMode = Desc.SyncMode;
    Body->PreviousTransform = Desc.WorldTransform;
    Body->CurrentTransform = Desc.WorldTransform;
    Body->bGenerateHitEvents = Desc.bGenerateHitEvents;
    Body->bGenerateOverlapEvents = Desc.bGenerateOverlapEvents;

    Body->State       = EPhysicsRuntimeObjectState::Alive;
    Body->Domain      = Desc.Domain;
    Body->CreatedStep = StepIndex;

    // mutable 속성 단일 소스 초기화 — 이후 SetMass/SetLock 등이 여기서 갱신되고,
    // RebuildBody 후에도 desc 를 통해 동일 값으로 재생성된다.
    Body->Properties.Mass                         = Desc.Mass;
    Body->Properties.CenterOfMassLocalOffset      = Desc.CenterOfMassLocalOffset;
    Body->Properties.LinearDamping                = Desc.LinearDamping;
    Body->Properties.AngularDamping               = Desc.AngularDamping;
    Body->Properties.MaxAngularVelocity           = Desc.MaxAngularVelocity;
    Body->Properties.PositionSolverIterationCount = Desc.PositionSolverIterationCount;
    Body->Properties.VelocitySolverIterationCount = Desc.VelocitySolverIterationCount;
    Body->Properties.bEnableCCD                   = Desc.bEnableCCD;
    Body->Properties.bEnableGravity               = Desc.bEnableGravity;
    Body->Properties.bLockLinearX                 = Desc.bLockLinearX;
    Body->Properties.bLockLinearY                 = Desc.bLockLinearY;
    Body->Properties.bLockLinearZ                 = Desc.bLockLinearZ;
    Body->Properties.bLockAngularX                = Desc.bLockAngularX;
    Body->Properties.bLockAngularY                = Desc.bLockAngularY;
    Body->Properties.bLockAngularZ                = Desc.bLockAngularZ;

    Actor->userData = Body;

    for (const FPhysicsShapeDesc& ShapeDesc : Desc.Shapes)
    {
        AddShapeToBody(BodyHandle, Desc.OwnerComponent, ShapeDesc);
    }

    if (Actor->is<PxRigidDynamic>())
    {
        FPhysXBodyBuilder::UpdateMassAndInertia(Actor, Desc);
    }

    Scene->addActor(*Actor);
    Body->bRegisteredInScene = true;

    return BodyHandle;
}

void FPhysXPhysicsRuntime::DestroyRigidBody_Internal(FPhysicsBodyHandle BodyHandle)
{
    FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body)
    {
        return;
    }

    // 이 시점부터 어떤 콜백/쿼리도 이 body 를 살아있다고 보면 안 된다.
    Body->State = EPhysicsRuntimeObjectState::PendingDestroy;

    TArray<FPhysicsConstraintHandle> ConstraintsToDestroy = Body->Constraints;
    for (FPhysicsConstraintHandle Constraint : ConstraintsToDestroy)
    {
        DestroyConstraint_Internal(Constraint);
    }

    TArray<FPhysicsShapeHandle> ShapesToDestroy = Body->Shapes;
    for (FPhysicsShapeHandle Shape : ShapesToDestroy)
    {
        DetachShape(Shape);
    }

    for (auto It = ComponentToBody.begin(); It != ComponentToBody.end();)
    {
        if (It->second == BodyHandle)
        {
            It = ComponentToBody.erase(It);
        }
        else
        {
            ++It;
        }
    }

    if (Body->Actor)
    {
        // release 전에 userData 를 끊어 지연된 callback/query 가 stale FBodyInstance 를
        // 역참조하지 못하게 한다.
        Body->Actor->userData = nullptr;

        if (Scene && Body->bRegisteredInScene)
        {
            Scene->removeActor(*Body->Actor);
        }

        Body->Actor->release();
        Body->Actor = nullptr;
    }

    Body->DestroyedStep = StepIndex;
    Body->State         = EPhysicsRuntimeObjectState::Destroyed;

    FreeBody(BodyHandle);
}

FPhysicsConstraintHandle FPhysXPhysicsRuntime::CreateConstraint_Internal(
    FPhysicsBodyHandle Parent,
    FPhysicsBodyHandle Child,
    const FConstraintCreationDesc& Desc
)
{
    FBodyInstance* ParentBody = ResolveBody(Parent);
    FBodyInstance* ChildBody = ResolveBody(Child);

    if (!ParentBody || !ChildBody || !ParentBody->Actor || !ChildBody->Actor)
    {
        return {};
    }

    PxJoint* Joint = FPhysXConstraintBuilder::CreateD6Joint(
        Physics,
        ParentBody->Actor,
        ChildBody->Actor,
        Desc
    );

    if (!Joint)
    {
        return {};
    }

    FPhysicsConstraintHandle Handle = AllocateConstraint();
    FConstraintInstance* Constraint = ResolveConstraint(Handle);
    if (!Constraint)
    {
        Joint->release();
        return {};
    }

    Constraint->ParentBody = Parent;
    Constraint->ChildBody = Child;
    Constraint->Joint = Joint;
    Constraint->ParentLocalFrame = Desc.ParentLocalFrame;
    Constraint->ChildLocalFrame = Desc.ChildLocalFrame;
    Constraint->Limits = Desc.Limits;
    Constraint->bDisableCollisionBetweenBodies = Desc.bDisableCollisionBetweenBodies;

    ParentBody->Constraints.push_back(Handle);
    ChildBody->Constraints.push_back(Handle);

    return Handle;
}

void FPhysXPhysicsRuntime::DestroyConstraint_Internal(FPhysicsConstraintHandle Handle)
{
    FConstraintInstance* Constraint = ResolveConstraint(Handle);
    if (!Constraint)
    {
        return;
    }

    if (FBodyInstance* ParentBody = ResolveBody(Constraint->ParentBody))
    {
        ParentBody->Constraints.erase(
            std::remove(ParentBody->Constraints.begin(), ParentBody->Constraints.end(), Handle),
            ParentBody->Constraints.end()
        );
    }

    if (FBodyInstance* ChildBody = ResolveBody(Constraint->ChildBody))
    {
        ChildBody->Constraints.erase(
            std::remove(ChildBody->Constraints.begin(), ChildBody->Constraints.end(), Handle),
            ChildBody->Constraints.end()
        );
    }

    if (Constraint->Joint)
    {
        Constraint->Joint->release();
        Constraint->Joint = nullptr;
    }

    FreeConstraint(Handle);
}

FPhysicsBodyHandle FPhysXPhysicsRuntime::CreateRigidBody(const FBodyCreationDesc& Desc)
{
    if (!Scene)
    {
        return {};
    }

    PxSceneWriteLock WriteLock(*Scene);
    return CreateRigidBody_Internal(Desc);
}

void FPhysXPhysicsRuntime::DestroyRigidBody(FPhysicsBodyHandle BodyHandle)
{
    FPhysicsCommand Cmd;
    Cmd.Type = EPhysicsCommandType::DestroyBody;
    Cmd.Body = BodyHandle;
    EnqueueCommand(Cmd);
}

FPhysicsConstraintHandle FPhysXPhysicsRuntime::CreateConstraint(
    FPhysicsBodyHandle             Parent,
    FPhysicsBodyHandle             Child,
    const FConstraintCreationDesc& Desc
)
{
    if (!Scene)
    {
        return {};
    }

    PxSceneWriteLock WriteLock(*Scene);
    return CreateConstraint_Internal(Parent, Child, Desc);
}

void FPhysXPhysicsRuntime::DestroyConstraint(FPhysicsConstraintHandle Constraint)
{
    FPhysicsCommand Cmd;
    Cmd.Type       = EPhysicsCommandType::DestroyConstraint;
    Cmd.Constraint = Constraint;
    EnqueueCommand(Cmd);
}

FTransform FPhysXPhysicsRuntime::GetBodyTransform(FPhysicsBodyHandle BodyHandle) const
{
    const FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return FTransform();
    }

    return Body->CurrentTransform;
}

void FPhysXPhysicsRuntime::SetBodyTransform(
    FPhysicsBodyHandle   BodyHandle,
    const FTransform&    Transform,
    EPhysicsTeleportMode TeleportMode
)
{
    FPhysicsCommand Cmd;
    Cmd.Type           = EPhysicsCommandType::SetTransform;
    Cmd.Body           = BodyHandle;
    Cmd.TransformValue = Transform;
    Cmd.TeleportMode   = TeleportMode;
    EnqueueCommand(Cmd);
}

void FPhysXPhysicsRuntime::ApplySetBodyTransform_Internal(
    FPhysicsBodyHandle BodyHandle,
    const FTransform&  Transform,
    EPhysicsTeleportMode
)
{
    FBodyInstance* Body = ResolveAliveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return;
    }

    Body->Actor->setGlobalPose(ToPxTransform(Transform));
    Body->PreviousTransform = Body->CurrentTransform;
    Body->CurrentTransform = Transform;
}

void FPhysXPhysicsRuntime::SetBodyVelocity(
    FPhysicsBodyHandle BodyHandle,
    const FVector& LinearVelocity,
    const FVector& AngularVelocity
)
{
    FPhysicsCommand Cmd;
    Cmd.Type         = EPhysicsCommandType::SetVelocity;
    Cmd.Body         = BodyHandle;
    Cmd.VectorValue  = LinearVelocity;
    Cmd.VectorValue2 = AngularVelocity;
    EnqueueCommand(Cmd);
}

void FPhysXPhysicsRuntime::ApplySetBodyVelocity_Internal(
    FPhysicsBodyHandle BodyHandle,
    const FVector&     LinearVelocity,
    const FVector&     AngularVelocity
)
{
    FBodyInstance* Body = ResolveAliveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return;
    }

    PxRigidDynamic* Dynamic = Body->Actor->is<PxRigidDynamic>();
    if (!Dynamic)
    {
        return;
    }

    Dynamic->setLinearVelocity(ToPxVec3(LinearVelocity));
    Dynamic->setAngularVelocity(ToPxVec3(AngularVelocity));
    Body->LinearVelocity  = LinearVelocity;
    Body->AngularVelocity = AngularVelocity;
}

FVector FPhysXPhysicsRuntime::GetBodyLinearVelocity(FPhysicsBodyHandle BodyHandle) const
{
    const FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return FVector::ZeroVector;
    }

    return Body->LinearVelocity;
}

FVector FPhysXPhysicsRuntime::GetBodyAngularVelocity(FPhysicsBodyHandle BodyHandle) const
{
    const FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return FVector::ZeroVector;
    }

    return Body->AngularVelocity;
}

void FPhysXPhysicsRuntime::AddForce(FPhysicsBodyHandle BodyHandle, const FVector& Force)
{
    FPhysicsCommand Cmd;
    Cmd.Type        = EPhysicsCommandType::AddForce;
    Cmd.Body        = BodyHandle;
    Cmd.VectorValue = Force;
    EnqueueCommand(Cmd);
}

void FPhysXPhysicsRuntime::ApplyAddForce_Internal(FPhysicsBodyHandle BodyHandle, const FVector& Force)
{
    FBodyInstance* Body = ResolveAliveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return;
    }

    PxRigidDynamic* Dynamic = Body->Actor->is<PxRigidDynamic>();
    if (!Dynamic)
    {
        return;
    }

    Dynamic->addForce(ToPxVec3(Force));
}

void FPhysXPhysicsRuntime::AddForceAtLocation(
    FPhysicsBodyHandle BodyHandle,
    const FVector& Force,
    const FVector& WorldLocation
)
{
    FPhysicsCommand Cmd;
    Cmd.Type         = EPhysicsCommandType::AddForceAtLocation;
    Cmd.Body         = BodyHandle;
    Cmd.VectorValue  = Force;
    Cmd.VectorValue2 = WorldLocation;
    EnqueueCommand(Cmd);
}

void FPhysXPhysicsRuntime::ApplyAddForceAtLocation_Internal(
    FPhysicsBodyHandle BodyHandle,
    const FVector&     Force,
    const FVector&     WorldLocation
)
{
    FBodyInstance* Body = ResolveAliveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return;
    }

    PxRigidDynamic* Dynamic = Body->Actor->is<PxRigidDynamic>();
    if (!Dynamic)
    {
        return;
    }

    PxRigidBodyExt::addForceAtPos(
        *Dynamic,
        ToPxVec3(Force),
        ToPxVec3(WorldLocation)
    );
}

void FPhysXPhysicsRuntime::AddTorque(FPhysicsBodyHandle BodyHandle, const FVector& Torque)
{
    FPhysicsCommand Cmd;
    Cmd.Type        = EPhysicsCommandType::AddTorque;
    Cmd.Body        = BodyHandle;
    Cmd.VectorValue = Torque;
    EnqueueCommand(Cmd);
}

void FPhysXPhysicsRuntime::ApplyAddTorque_Internal(FPhysicsBodyHandle BodyHandle, const FVector& Torque)
{
    FBodyInstance* Body = ResolveAliveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return;
    }

    PxRigidDynamic* Dynamic = Body->Actor->is<PxRigidDynamic>();
    if (!Dynamic)
    {
        return;
    }

    Dynamic->addTorque(ToPxVec3(Torque));
}

void FPhysXPhysicsRuntime::AddImpulse(FPhysicsBodyHandle BodyHandle, const FVector& Impulse)
{
    FPhysicsCommand Cmd;
    Cmd.Type        = EPhysicsCommandType::AddImpulse;
    Cmd.Body        = BodyHandle;
    Cmd.VectorValue = Impulse;
    EnqueueCommand(Cmd);
}

void FPhysXPhysicsRuntime::ApplyAddImpulse_Internal(FPhysicsBodyHandle BodyHandle, const FVector& Impulse)
{
    FBodyInstance* Body = ResolveAliveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return;
    }

    PxRigidDynamic* Dynamic = Body->Actor->is<PxRigidDynamic>();
    if (!Dynamic)
    {
        return;
    }

    Dynamic->addForce(ToPxVec3(Impulse), PxForceMode::eIMPULSE);
}

void FPhysXPhysicsRuntime::SetMass(FPhysicsBodyHandle BodyHandle, float Mass)
{
    FPhysicsCommand Cmd;
    Cmd.Type       = EPhysicsCommandType::SetMass;
    Cmd.Body       = BodyHandle;
    Cmd.FloatValue = Mass;
    EnqueueCommand(Cmd);
}

void FPhysXPhysicsRuntime::ApplySetMass_Internal(FPhysicsBodyHandle BodyHandle, float Mass)
{
    FBodyInstance* Body = ResolveAliveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return;
    }

    // 단일 소스(Properties)에 반영하고, 저장된 COM 을 함께 적용해 서로 덮어쓰지 않게 한다.
    Body->Properties.Mass = (Mass > 0.0f) ? Mass : Body->Properties.Mass;

    FBodyCreationDesc Desc;
    Desc.Mass                    = Body->Properties.Mass;
    Desc.CenterOfMassLocalOffset = Body->Properties.CenterOfMassLocalOffset;
    FPhysXBodyBuilder::UpdateMassAndInertia(Body->Actor, Desc);
}

float FPhysXPhysicsRuntime::GetMass(FPhysicsBodyHandle BodyHandle) const
{
    const FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return 1.0f;
    }

    return Body->Properties.Mass;
}

void FPhysXPhysicsRuntime::SetCenterOfMass(FPhysicsBodyHandle BodyHandle, const FVector& LocalOffset)
{
    FPhysicsCommand Cmd;
    Cmd.Type        = EPhysicsCommandType::SetCenterOfMass;
    Cmd.Body        = BodyHandle;
    Cmd.VectorValue = LocalOffset;
    EnqueueCommand(Cmd);
}

void FPhysXPhysicsRuntime::ApplySetCenterOfMass_Internal(FPhysicsBodyHandle BodyHandle, const FVector& LocalOffset)
{
    FBodyInstance* Body = ResolveAliveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return;
    }

    Body->Properties.CenterOfMassLocalOffset = LocalOffset;

    PxRigidDynamic* Dynamic = Body->Actor->is<PxRigidDynamic>();
    if (!Dynamic)
    {
        return;
    }

    Dynamic->setCMassLocalPose(PxTransform(ToPxVec3(LocalOffset)));
}

FVector FPhysXPhysicsRuntime::GetCenterOfMass(FPhysicsBodyHandle BodyHandle) const
{
    const FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return FVector::ZeroVector;
    }

    return Body->Properties.CenterOfMassLocalOffset;
}

void FPhysXPhysicsRuntime::SetLinearLock(FPhysicsBodyHandle BodyHandle, bool bX, bool bY, bool bZ)
{
    FPhysicsCommand Cmd;
    Cmd.Type  = EPhysicsCommandType::SetLinearLock;
    Cmd.Body  = BodyHandle;
    Cmd.BoolX = bX;
    Cmd.BoolY = bY;
    Cmd.BoolZ = bZ;
    EnqueueCommand(Cmd);
}

void FPhysXPhysicsRuntime::ApplySetLinearLock_Internal(FPhysicsBodyHandle BodyHandle, bool bX, bool bY, bool bZ)
{
    FBodyInstance* Body = ResolveAliveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return;
    }

    Body->Properties.bLockLinearX = bX;
    Body->Properties.bLockLinearY = bY;
    Body->Properties.bLockLinearZ = bZ;

    PxRigidDynamic* Dynamic = Body->Actor->is<PxRigidDynamic>();
    if (!Dynamic)
    {
        return;
    }

    Dynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_X, bX);
    Dynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, bY);
    Dynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, bZ);
}

void FPhysXPhysicsRuntime::SetAngularLock(FPhysicsBodyHandle BodyHandle, bool bX, bool bY, bool bZ)
{
    FPhysicsCommand Cmd;
    Cmd.Type  = EPhysicsCommandType::SetAngularLock;
    Cmd.Body  = BodyHandle;
    Cmd.BoolX = bX;
    Cmd.BoolY = bY;
    Cmd.BoolZ = bZ;
    EnqueueCommand(Cmd);
}

void FPhysXPhysicsRuntime::ApplySetAngularLock_Internal(FPhysicsBodyHandle BodyHandle, bool bX, bool bY, bool bZ)
{
    FBodyInstance* Body = ResolveAliveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return;
    }

    Body->Properties.bLockAngularX = bX;
    Body->Properties.bLockAngularY = bY;
    Body->Properties.bLockAngularZ = bZ;

    PxRigidDynamic* Dynamic = Body->Actor->is<PxRigidDynamic>();
    if (!Dynamic)
    {
        return;
    }

    Dynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, bX);
    Dynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, bY);
    Dynamic->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, bZ);
}

void FPhysXPhysicsRuntime::BuildDebugBodies_Internal(TArray<FPhysicsDebugBody>& OutBodies) const
{
    OutBodies.clear();

    for (const auto& BodyPtr : Bodies)
    {
        if (!BodyPtr || !BodyPtr->Actor)
        {
            continue;
        }

        const FBodyInstance& Body = *BodyPtr;

        FPhysicsDebugBody DebugBody;
        DebugBody.Body                      = Body.Handle;
        DebugBody.BodyWorldTransform        = ToFTransform(Body.Actor->getGlobalPose());
        AActor*              OwnerActor     = Body.OwnerActor.Get();
        UPrimitiveComponent* OwnerComponent = Body.OwnerComponent.Get();
        DebugBody.OwnerActor                = Body.OwnerActor;
        DebugBody.OwnerComponent            = Body.OwnerComponent;
        DebugBody.OwnerActorId              = OwnerActor ? OwnerActor->GetUUID() : 0;
        DebugBody.OwnerComponentId          = OwnerComponent ? OwnerComponent->GetUUID() : 0;
        DebugBody.BoneName                  = Body.BoneName;

        DebugBody.bIsStatic = Body.BodyType == EPhysicsBodyType::Static;
        DebugBody.bIsDynamic = Body.BodyType == EPhysicsBodyType::Dynamic;
        DebugBody.bIsKinematic = Body.BodyType == EPhysicsBodyType::Kinematic;

        if (PxRigidDynamic* Dynamic = Body.Actor->is<PxRigidDynamic>())
        {
            DebugBody.bIsSleeping = Dynamic->isSleeping();
        }

        for (FPhysicsShapeHandle ShapeHandle : Body.Shapes)
        {
            const FShapeInstance* ShapeInstance = ResolveShape(ShapeHandle);
            if (!ShapeInstance || ShapeInstance->State != EPhysicsRuntimeObjectState::Alive)
            {
                continue;
            }

            FPhysicsDebugShape DebugShape;
            DebugShape.Shape = ShapeHandle;
            DebugShape.Type = ShapeInstance->Desc.Type;
            DebugShape.BoxHalfExtent = ShapeInstance->Desc.BoxHalfExtent;
            DebugShape.SphereRadius = ShapeInstance->Desc.SphereRadius;
            DebugShape.CapsuleRadius = ShapeInstance->Desc.CapsuleRadius;
            DebugShape.CapsuleHalfHeight = ShapeInstance->Desc.CapsuleHalfHeight;

            // PhysX shape에 실제 적용된 local pose를 body 월드 transform으로 합성한다.
            // Capsule 축 보정 등 backend local pose 보정까지 포함되어 debug draw와 실제 충돌이 일치한다.
            DebugShape.WorldTransform = ComposePhysicsTransforms(
                DebugBody.BodyWorldTransform,
                ShapeInstance->PhysXLocalTransform
            );

            DebugBody.Shapes.push_back(DebugShape);
        }

        OutBodies.push_back(DebugBody);
    }
}

void FPhysXPhysicsRuntime::BuildDebugConstraints_Internal(TArray<FPhysicsDebugConstraint>& OutConstraints) const
{
    OutConstraints.clear();

    for (const auto& ConstraintPtr : Constraints)
    {
        if (!ConstraintPtr || !ConstraintPtr->Joint)
        {
            continue;
        }

        const FConstraintInstance& Constraint = *ConstraintPtr;

        const FBodyInstance* Parent = ResolveBody(Constraint.ParentBody);
        const FBodyInstance* Child = ResolveBody(Constraint.ChildBody);

        if (!Parent || !Child || !Parent->Actor || !Child->Actor)
        {
            continue;
        }

        FPhysicsDebugConstraint Debug;
        Debug.Constraint = Constraint.Handle;
        Debug.ParentBody = Constraint.ParentBody;
        Debug.ChildBody = Constraint.ChildBody;

        const FTransform ParentWorld = ToFTransform(Parent->Actor->getGlobalPose());
        const FTransform ChildWorld = ToFTransform(Child->Actor->getGlobalPose());

        // joint frame 의 월드 변환도 동일 규칙 (BodyWorld * LocalFrame) 으로 합성한다.
        Debug.ParentFrameWorld = ComposePhysicsTransforms(ParentWorld, Constraint.ParentLocalFrame);
        Debug.ChildFrameWorld = ComposePhysicsTransforms(ChildWorld, Constraint.ChildLocalFrame);

        Debug.TwistLimitMinDegrees = Constraint.Limits.TwistLimitMinDegrees;
        Debug.TwistLimitMaxDegrees = Constraint.Limits.TwistLimitMaxDegrees;
        Debug.Swing1LimitDegrees = Constraint.Limits.Swing1LimitDegrees;
        Debug.Swing2LimitDegrees = Constraint.Limits.Swing2LimitDegrees;

        OutConstraints.push_back(Debug);
    }
}

void FPhysXPhysicsRuntime::BuildDebugSnapshot_Internal()
{
    FPhysicsDebugSnapshot NewSnapshot;
    BuildDebugBodies_Internal(NewSnapshot.Bodies);
    BuildDebugConstraints_Internal(NewSnapshot.Constraints);
    NewSnapshot.Stats              = Stats;
    NewSnapshot.InterpolationAlpha = Stats.InterpolationAlpha;
    NewSnapshot.StepIndex          = StepIndex;

    std::lock_guard<std::mutex> Lock(DebugSnapshotMutex);
    DebugSnapshot = std::move(NewSnapshot);
}

void FPhysXPhysicsRuntime::GatherDebugBodies(TArray<FPhysicsDebugBody>& OutBodies) const
{
    std::lock_guard<std::mutex> Lock(DebugSnapshotMutex);
    OutBodies = DebugSnapshot.Bodies;
}

void FPhysXPhysicsRuntime::GatherDebugConstraints(TArray<FPhysicsDebugConstraint>& OutConstraints) const
{
    std::lock_guard<std::mutex> Lock(DebugSnapshotMutex);
    OutConstraints = DebugSnapshot.Constraints;
}

void FPhysXPhysicsRuntime::GetDebugSnapshot(FPhysicsDebugSnapshot& OutSnapshot) const
{
    std::lock_guard<std::mutex> Lock(DebugSnapshotMutex);
    OutSnapshot = DebugSnapshot;
}

FPhysicsBodyHandle FPhysXPhysicsRuntime::AllocateBody()
{
    for (uint32 i = 0; i < static_cast<uint32>(Bodies.size()); ++i)
    {
        if (!Bodies[i])
        {
            Bodies[i] = std::make_unique<FBodyInstance>();
            FPhysicsBodyHandle Handle;
            Handle.Index = i;
            Handle.Generation = BodyGenerations[i];
            Bodies[i]->Handle = Handle;
            return Handle;
        }
    }

    const uint32 NewIndex = static_cast<uint32>(Bodies.size());
    Bodies.push_back(std::make_unique<FBodyInstance>());
    BodyGenerations.push_back(1);

    FPhysicsBodyHandle Handle;
    Handle.Index = NewIndex;
    Handle.Generation = BodyGenerations[NewIndex];
    Bodies[NewIndex]->Handle = Handle;
    return Handle;
}

FPhysicsShapeHandle FPhysXPhysicsRuntime::AllocateShape()
{
    for (uint32 i = 0; i < static_cast<uint32>(Shapes.size()); ++i)
    {
        if (!Shapes[i])
        {
            Shapes[i] = std::make_unique<FShapeInstance>();
            FPhysicsShapeHandle Handle;
            Handle.Index = i;
            Handle.Generation = ShapeGenerations[i];
            Shapes[i]->Handle = Handle;
            return Handle;
        }
    }

    const uint32 NewIndex = static_cast<uint32>(Shapes.size());
    Shapes.push_back(std::make_unique<FShapeInstance>());
    ShapeGenerations.push_back(1);

    FPhysicsShapeHandle Handle;
    Handle.Index = NewIndex;
    Handle.Generation = ShapeGenerations[NewIndex];
    Shapes[NewIndex]->Handle = Handle;
    return Handle;
}

FPhysicsConstraintHandle FPhysXPhysicsRuntime::AllocateConstraint()
{
    for (uint32 i = 0; i < static_cast<uint32>(Constraints.size()); ++i)
    {
        if (!Constraints[i])
        {
            Constraints[i] = std::make_unique<FConstraintInstance>();
            FPhysicsConstraintHandle Handle;
            Handle.Index = i;
            Handle.Generation = ConstraintGenerations[i];
            Constraints[i]->Handle = Handle;
            return Handle;
        }
    }

    const uint32 NewIndex = static_cast<uint32>(Constraints.size());
    Constraints.push_back(std::make_unique<FConstraintInstance>());
    ConstraintGenerations.push_back(1);

    FPhysicsConstraintHandle Handle;
    Handle.Index = NewIndex;
    Handle.Generation = ConstraintGenerations[NewIndex];
    Constraints[NewIndex]->Handle = Handle;
    return Handle;
}

FBodyInstance* FPhysXPhysicsRuntime::ResolveBody(FPhysicsBodyHandle Handle)
{
    if (!IsHandleIndexValid(Handle.Index, Bodies.size()) ||
        BodyGenerations[Handle.Index] != Handle.Generation)
    {
        return nullptr;
    }

    return Bodies[Handle.Index].get();
}

const FBodyInstance* FPhysXPhysicsRuntime::ResolveBody(FPhysicsBodyHandle Handle) const
{
    if (!IsHandleIndexValid(Handle.Index, Bodies.size()) ||
        BodyGenerations[Handle.Index] != Handle.Generation)
    {
        return nullptr;
    }

    return Bodies[Handle.Index].get();
}

FBodyInstance* FPhysXPhysicsRuntime::ResolveAliveBody(FPhysicsBodyHandle Handle)
{
    FBodyInstance* Body = ResolveBody(Handle);
    return (Body && Body->State == EPhysicsRuntimeObjectState::Alive) ? Body : nullptr;
}

const FBodyInstance* FPhysXPhysicsRuntime::ResolveAliveBody(FPhysicsBodyHandle Handle) const
{
    const FBodyInstance* Body = ResolveBody(Handle);
    return (Body && Body->State == EPhysicsRuntimeObjectState::Alive) ? Body : nullptr;
}

FShapeInstance* FPhysXPhysicsRuntime::ResolveShape(FPhysicsShapeHandle Handle)
{
    if (!IsHandleIndexValid(Handle.Index, Shapes.size()) ||
        ShapeGenerations[Handle.Index] != Handle.Generation)
    {
        return nullptr;
    }

    return Shapes[Handle.Index].get();
}

const FShapeInstance* FPhysXPhysicsRuntime::ResolveShape(FPhysicsShapeHandle Handle) const
{
    if (!IsHandleIndexValid(Handle.Index, Shapes.size()) ||
        ShapeGenerations[Handle.Index] != Handle.Generation)
    {
        return nullptr;
    }

    return Shapes[Handle.Index].get();
}

FConstraintInstance* FPhysXPhysicsRuntime::ResolveConstraint(FPhysicsConstraintHandle Handle)
{
    if (!IsHandleIndexValid(Handle.Index, Constraints.size()) ||
        ConstraintGenerations[Handle.Index] != Handle.Generation)
    {
        return nullptr;
    }

    return Constraints[Handle.Index].get();
}

const FConstraintInstance* FPhysXPhysicsRuntime::ResolveConstraint(FPhysicsConstraintHandle Handle) const
{
    if (!IsHandleIndexValid(Handle.Index, Constraints.size()) ||
        ConstraintGenerations[Handle.Index] != Handle.Generation)
    {
        return nullptr;
    }

    return Constraints[Handle.Index].get();
}

void FPhysXPhysicsRuntime::FreeBody(FPhysicsBodyHandle Handle)
{
    if (!IsHandleIndexValid(Handle.Index, Bodies.size()))
    {
        return;
    }

    Bodies[Handle.Index].reset();
    ++BodyGenerations[Handle.Index];
}

void FPhysXPhysicsRuntime::FreeShape(FPhysicsShapeHandle Handle)
{
    if (!IsHandleIndexValid(Handle.Index, Shapes.size()))
    {
        return;
    }

    Shapes[Handle.Index].reset();
    ++ShapeGenerations[Handle.Index];
}

void FPhysXPhysicsRuntime::FreeConstraint(FPhysicsConstraintHandle Handle)
{
    if (!IsHandleIndexValid(Handle.Index, Constraints.size()))
    {
        return;
    }

    Constraints[Handle.Index].reset();
    ++ConstraintGenerations[Handle.Index];
}

FBodyCreationDesc FPhysXPhysicsRuntime::BuildBodyDescFromComponent(UPrimitiveComponent* Comp) const
{
    FBodyCreationDesc Desc;

    if (!Comp)
    {
        return Desc;
    }

    Desc.OwnerActor = Comp->GetOwner();
    Desc.OwnerComponent = Comp;
    Desc.WorldTransform = GetComponentWorldTransform(Comp);

    const bool bSimulate = Comp->GetSimulatePhysics();
    const bool bKinematic = !bSimulate && Comp->IsKinematic();

    if (bSimulate)
    {
        Desc.BodyType = EPhysicsBodyType::Dynamic;
        Desc.SyncMode = EPhysicsSyncMode::PhysicsToEngine;
    }
    else if (bKinematic)
    {
        Desc.BodyType = EPhysicsBodyType::Kinematic;
        Desc.SyncMode = EPhysicsSyncMode::KinematicTarget;
    }
    else
    {
        Desc.BodyType = EPhysicsBodyType::Static;
        Desc.SyncMode = EPhysicsSyncMode::EngineToPhysics;
    }

    Desc.Mass = Comp->GetMass();
    Desc.CenterOfMassLocalOffset = Comp->GetCenterOfMass();
    Desc.bGenerateHitEvents = true;
    Desc.bGenerateOverlapEvents = Comp->GetGenerateOverlapEvents();

    // DOF lock 은 RootComponent 정책 (Mass/COM 과 동일). RebuildBody 후에도 유지되도록
    // 컴포넌트에 저장된 lock 상태를 desc 로 전달한다.
    Desc.bLockLinearX = Comp->IsLinearLockX();
    Desc.bLockLinearY = Comp->IsLinearLockY();
    Desc.bLockLinearZ = Comp->IsLinearLockZ();
    Desc.bLockAngularX = Comp->IsAngularLockX();
    Desc.bLockAngularY = Comp->IsAngularLockY();
    Desc.bLockAngularZ = Comp->IsAngularLockZ();

    return Desc;
}

FPhysicsShapeDesc FPhysXPhysicsRuntime::BuildShapeDescFromComponent(
    UPrimitiveComponent* Comp,
    UPrimitiveComponent* RootComponent
) const
{
    FPhysicsShapeDesc Desc;

    if (!Comp)
    {
        return Desc;
    }

    Desc.LocalTransform = MakeShapeLocalTransform(Comp, RootComponent);
    Desc.CollisionEnabled = Comp->GetCollisionEnabled();

    const bool bOwnerBodyIsDynamic =
        RootComponent &&
        (RootComponent->GetSimulatePhysics() || RootComponent->IsKinematic());

    Desc.bIsTrigger = ShouldBeTriggerShape(Comp, bOwnerBodyIsDynamic);
    FillFilterDataFromComponent(Desc.FilterData, Comp, Desc.bIsTrigger);

    if (auto* Box = Cast<UBoxComponent>(Comp))
    {
        Desc.Type = EPhysicsShapeType::Box;
        Desc.BoxHalfExtent = Box->GetScaledBoxExtent();
    }
    else if (auto* Sphere = Cast<USphereComponent>(Comp))
    {
        Desc.Type = EPhysicsShapeType::Sphere;
        Desc.SphereRadius = Sphere->GetScaledSphereRadius();
    }
    else if (auto* Capsule = Cast<UCapsuleComponent>(Comp))
    {
        Desc.Type = EPhysicsShapeType::Capsule;
        Desc.CapsuleRadius = Capsule->GetScaledCapsuleRadius();
        Desc.CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    }
    else
    {
        // StaticMesh/SkeletalMesh 등 별도 BodySetup이 아직 없는 PrimitiveComponent는
        // 현재 엔진이 이미 제공하는 world AABB를 이용해 box proxy로 등록합니다.
        const FBoundingBox Bounds = Comp->GetWorldBoundingBox();
        const FVector WorldCenter = Bounds.GetCenter();
        FVector WorldExtent = Bounds.GetExtent();

        if (WorldExtent.X <= 0.0f || WorldExtent.Y <= 0.0f || WorldExtent.Z <= 0.0f)
        {
            WorldExtent = FVector(0.5f, 0.5f, 0.5f);
        }

        Desc.Type = EPhysicsShapeType::Box;
        Desc.BoxHalfExtent = WorldExtent;
        Desc.LocalTransform = MakeRelativeTransformFromWorld(
            WorldCenter,
            Comp->GetWorldMatrix().ToQuat(),
            RootComponent
        );
    }

    return Desc;
}

FPhysicsShapeHandle FPhysXPhysicsRuntime::AddShapeToBody(
    FPhysicsBodyHandle BodyHandle,
    UPrimitiveComponent* SourceComponent,
    const FPhysicsShapeDesc& Desc
)
{
    FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body || !Body->Actor || !Physics || !DefaultMaterial)
    {
        return {};
    }

    PxShape* Shape = FPhysXBodyBuilder::CreateShape(Physics, DefaultMaterial, Desc);
    if (!Shape)
    {
        return {};
    }

    Body->Actor->attachShape(*Shape);

    FPhysicsShapeHandle ShapeHandle = AllocateShape();
    FShapeInstance* ShapeInstance = ResolveShape(ShapeHandle);
    if (!ShapeInstance)
    {
        Body->Actor->detachShape(*Shape);
        Shape->release();
        return {};
    }

    ShapeInstance->OwnerBody            = BodyHandle;
    ShapeInstance->SourceComponent      = SourceComponent;
    ShapeInstance->Desc                 = Desc;
    ShapeInstance->EngineLocalTransform = Desc.LocalTransform;
    ShapeInstance->PhysXLocalTransform  = ToFTransform(Shape->getLocalPose());
    ShapeInstance->Shape                = Shape;
    ShapeInstance->State                = EPhysicsRuntimeObjectState::Alive;

    Shape->userData = ShapeInstance;
    Body->Shapes.push_back(ShapeHandle);

    // createShape() returns an owning reference. attachShape() adds the actor's
    // reference, so drop the local reference here and let the actor own the
    // shape until detach/release. ShapeInstance keeps a non-owning pointer.
    Shape->release();

    return ShapeHandle;
}

void FPhysXPhysicsRuntime::DetachShape(FPhysicsShapeHandle ShapeHandle)
{
    FShapeInstance* ShapeInstance = ResolveShape(ShapeHandle);
    if (!ShapeInstance)
    {
        return;
    }

    FBodyInstance* Body = ResolveBody(ShapeInstance->OwnerBody);
    if (Body && Body->Actor && ShapeInstance->Shape)
    {
        // detach 하면 actor 의 마지막 ref 가 빠지면서 PxShape 가 즉시 release 될 수 있으므로,
        // 그 전에 userData 를 끊어 지연된 callback/query 의 stale 역참조를 막는다.
        ShapeInstance->Shape->userData = nullptr;

        Body->Actor->detachShape(*ShapeInstance->Shape);
        Body->Shapes.erase(
            std::remove(Body->Shapes.begin(), Body->Shapes.end(), ShapeHandle),
            Body->Shapes.end()
        );
    }

    if (UPrimitiveComponent* SourceComponent = ShapeInstance->SourceComponent.GetEvenIfPendingKill())
    {
        ComponentToShape.erase(GetObjectKey(SourceComponent));
    }
    ShapeInstance->State = EPhysicsRuntimeObjectState::Destroyed;

    FreeShape(ShapeHandle);
}

void FPhysXPhysicsRuntime::DetachShapeForComponent(UPrimitiveComponent* Comp)
{
    auto It = ComponentToShape.find(GetObjectKey(Comp));
    if (It == ComponentToShape.end())
    {
        return;
    }

    DetachShape(It->second);
}

void FPhysXPhysicsRuntime::SyncEngineToPhysics(FBodyInstance& Body)
{
    if (!Body.Actor || !IsValid(Body.OwnerComponent.Get()))
    {
        return;
    }

    const FTransform  Target   = GetComponentWorldTransform(Body.OwnerComponent.Get());
    const PxTransform PxTarget = ToPxTransform(Target);

    if (PxRigidDynamic* Dynamic = Body.Actor->is<PxRigidDynamic>())
    {
        if (Dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC)
        {
            Dynamic->setKinematicTarget(PxTarget);
        }
        else
        {
            Dynamic->setGlobalPose(PxTarget);
        }
    }
    else
    {
        Body.Actor->setGlobalPose(PxTarget);
    }

    Body.PreviousTransform = Body.CurrentTransform;
    Body.CurrentTransform = Target;
}

void FPhysXPhysicsRuntime::SyncPhysicsToEngine(FBodyInstance& Body)
{
    if (!Body.Actor || !IsValid(Body.OwnerComponent.Get()))
    {
        return;
    }

    PxRigidDynamic* Dynamic = Body.Actor->is<PxRigidDynamic>();
    if (!Dynamic || (Dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC))
    {
        return;
    }

    const PxTransform Pose = Dynamic->getGlobalPose();
    Body.PreviousTransform = Body.CurrentTransform;
    Body.CurrentTransform  = ToFTransform(Pose);
    Body.LinearVelocity    = ToFVector(Dynamic->getLinearVelocity());
    Body.AngularVelocity   = ToFVector(Dynamic->getAngularVelocity());
    Body.bIsSleeping       = Dynamic->isSleeping();

    Body.OwnerComponent.Get()->SetWorldLocation(Body.CurrentTransform.Location);
    Body.OwnerComponent.Get()->SetRelativeRotation(Body.CurrentTransform.Rotation);
}

void FPhysXPhysicsRuntime::EnqueueCommand(const FPhysicsCommand& Command)
{
    CommandQueue.Enqueue(Command);
}

int32 FPhysXPhysicsRuntime::ApplyPendingCommands()
{
    TArray<FPhysicsCommand> Commands;
    CommandQueue.Drain(Commands);

    // Drain 은 enqueue(=Sequence) 순서를 보존하므로 같은 frame 내 명령 순서가 보장된다.
    for (const FPhysicsCommand& Command : Commands)
    {
        switch (Command.Type)
        {
        case EPhysicsCommandType::RegisterComponent:
            RegisterComponent_Internal(Command.Component.GetEvenIfPendingKill());
            break;
        case EPhysicsCommandType::UnregisterComponent:
            UnregisterComponent_Internal(Command.Component.GetEvenIfPendingKill());
            break;
        case EPhysicsCommandType::RebuildComponentBody:
            RebuildBody_Internal(Command.Component.Get());
            break;
        case EPhysicsCommandType::DestroyBody:
            DestroyRigidBody_Internal(Command.Body);
            break;
        case EPhysicsCommandType::DestroyConstraint:
            DestroyConstraint_Internal(Command.Constraint);
            break;
        case EPhysicsCommandType::SetTransform:
        case EPhysicsCommandType::SetKinematicTarget:
            ApplySetBodyTransform_Internal(Command.Body, Command.TransformValue, Command.TeleportMode);
            break;
        case EPhysicsCommandType::AddForce:
            ApplyAddForce_Internal(Command.Body, Command.VectorValue);
            break;
        case EPhysicsCommandType::AddForceAtLocation:
            ApplyAddForceAtLocation_Internal(Command.Body, Command.VectorValue, Command.VectorValue2);
            break;
        case EPhysicsCommandType::AddTorque:
            ApplyAddTorque_Internal(Command.Body, Command.VectorValue);
            break;
        case EPhysicsCommandType::AddImpulse:
            ApplyAddImpulse_Internal(Command.Body, Command.VectorValue);
            break;
        case EPhysicsCommandType::SetVelocity:
            ApplySetBodyVelocity_Internal(Command.Body, Command.VectorValue, Command.VectorValue2);
            break;
        case EPhysicsCommandType::SetLinearVelocity:
            ApplySetBodyVelocity_Internal(Command.Body, Command.VectorValue, GetBodyAngularVelocity(Command.Body));
            break;
        case EPhysicsCommandType::SetAngularVelocity:
            ApplySetBodyVelocity_Internal(Command.Body, GetBodyLinearVelocity(Command.Body), Command.VectorValue);
            break;
        case EPhysicsCommandType::SetMass:
            ApplySetMass_Internal(Command.Body, Command.FloatValue);
            break;
        case EPhysicsCommandType::SetCenterOfMass:
            ApplySetCenterOfMass_Internal(Command.Body, Command.VectorValue);
            break;
        case EPhysicsCommandType::SetLinearLock:
            ApplySetLinearLock_Internal(Command.Body, Command.BoolX, Command.BoolY, Command.BoolZ);
            break;
        case EPhysicsCommandType::SetAngularLock:
            ApplySetAngularLock_Internal(Command.Body, Command.BoolX, Command.BoolY, Command.BoolZ);
            break;
        default:
            break;
        }
    }

    return static_cast<int32>(Commands.size());
}

void FPhysXPhysicsRuntime::UpdateStats()
{
    const int32 NumSubsteps = Stats.NumSubsteps;
    Stats = FPhysicsStats();
    Stats.NumSubsteps = NumSubsteps;

    for (const auto& BodyPtr : Bodies)
    {
        if (!BodyPtr || !BodyPtr->Actor)
        {
            continue;
        }

        ++Stats.NumBodies;

        if (BodyPtr->BodyType == EPhysicsBodyType::Static)
        {
            ++Stats.NumStaticBodies;
        }
        else if (BodyPtr->BodyType == EPhysicsBodyType::Dynamic)
        {
            ++Stats.NumDynamicBodies;
        }
        else if (BodyPtr->BodyType == EPhysicsBodyType::Kinematic)
        {
            ++Stats.NumKinematicBodies;
        }

        Stats.NumShapes += static_cast<int32>(BodyPtr->Shapes.size());

        if (PxRigidDynamic* Dynamic = BodyPtr->Actor->is<PxRigidDynamic>())
        {
            if (Dynamic->isSleeping())
            {
                ++Stats.NumSleepingBodies;
            }
            else
            {
                ++Stats.NumActiveBodies;
            }
        }
    }

    for (const auto& ConstraintPtr : Constraints)
    {
        if (ConstraintPtr && ConstraintPtr->Joint)
        {
            ++Stats.NumConstraints;
        }
    }
}

FActorCompoundBody* FPhysXPhysicsRuntime::FindCompoundByActor(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return nullptr;
    }

    auto It = ActorCompounds.find(GetObjectKey(Actor));
    return It != ActorCompounds.end() ? &It->second : nullptr;
}

const FActorCompoundBody* FPhysXPhysicsRuntime::FindCompoundByActor(AActor* Actor) const
{
    if (!IsValid(Actor))
    {
        return nullptr;
    }

    auto It = ActorCompounds.find(GetObjectKey(Actor));
    return It != ActorCompounds.end() ? &It->second : nullptr;
}
