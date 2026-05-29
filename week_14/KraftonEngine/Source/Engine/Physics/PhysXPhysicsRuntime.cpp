#include "Physics/PhysXPhysicsRuntime.h"
#include "Physics/PhysXBodyBuilder.h"
#include "Physics/PhysXConstraintBuilder.h"
#include "Physics/PhysXConversion.h"

#include "Component/PrimitiveComponent.h"
#include "Component/Shape/BoxComponent.h"
#include "Component/Shape/CapsuleComponent.h"
#include "Component/Shape/SphereComponent.h"
#include "Core/Logging/Log.h"
#include "Core/Types/EngineTypes.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"
#include "Object/Object.h"

#include <algorithm>
#include <PxPhysicsAPI.h>

using namespace physx;

namespace
{
    bool IsHandleIndexValid(uint32 Index, size_t Size)
    {
        return Index != UINT32_MAX && static_cast<size_t>(Index) < Size;
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

    void FillFilterDataFromComponent(
        FPhysicsFilterData& Out,
        UPrimitiveComponent* Comp
    )
    {
        if (!Comp)
        {
            return;
        }

        Out.ObjectType = static_cast<uint32>(Comp->GetCollisionObjectType());
        Out.BlockMask = 0;
        Out.OverlapMask = 0;
        Out.IgnoreGroup = Comp->GetOwner() ? Comp->GetOwner()->GetUUID() : 0;

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

    bool ShouldBeTriggerShape(UPrimitiveComponent* Comp, bool bOwnerBodyIsDynamic)
    {
        if (!Comp)
        {
            return false;
        }

        bool bShouldBeTrigger = Comp->GetGenerateOverlapEvents();

        if (!bShouldBeTrigger)
        {
            bool bHasAnyBlockResponse = false;
            for (int32 Ch = 0; Ch < static_cast<int32>(ECollisionChannel::ActiveCount); ++Ch)
            {
                if (Comp->GetCollisionResponseToChannel(static_cast<ECollisionChannel>(Ch))
                    == ECollisionResponse::Block)
                {
                    bHasAnyBlockResponse = true;
                    break;
                }
            }

            bShouldBeTrigger = !bHasAnyBlockResponse;
        }

        // PhysX는 trigger-trigger pair에 onTrigger를 발화하지 않습니다. 움직이는 kinematic/dynamic
        // 캐릭터 capsule은 static trigger volume에 감지되어야 하므로 dynamic actor의 shape는
        // simulation shape로 유지합니다.
        if (bShouldBeTrigger && bOwnerBodyIsDynamic)
        {
            bShouldBeTrigger = false;
        }

        return bShouldBeTrigger;
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
    Accumulator = 0.0f;
    Stats = FPhysicsStats();
}

void FPhysXPhysicsRuntime::Shutdown()
{
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
        UpdateStats();
        return;
    }

    if (DeltaTime > MaxFrameDt)
    {
        DeltaTime = MaxFrameDt;
    }

    Accumulator += DeltaTime;

    int32 StepCount = 0;

    while (Accumulator >= FixedDt && StepCount < MaxSubsteps)
    {
        ApplyPendingCommands();

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

        Scene->simulate(FixedDt);
        Scene->fetchResults(true);

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
                Body.CurrentTransform = GetBodyTransform(Body.Handle);
            }
        }

        Accumulator -= FixedDt;
        ++StepCount;
    }

    if (StepCount == MaxSubsteps)
    {
        Accumulator = 0.0f;
    }

    Stats.NumSubsteps = StepCount;
    UpdateStats();
}

void FPhysXPhysicsRuntime::RegisterComponent(UPrimitiveComponent* Comp)
{
    if (!IsValid(Comp) || !Physics || !Scene || !DefaultMaterial)
    {
        return;
    }

    if (ComponentToShape.find(Comp) != ComponentToShape.end())
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

        FPhysicsBodyHandle BodyHandle = CreateRigidBody(BodyDesc);
        if (!BodyHandle.IsValid())
        {
            return;
        }

        FActorCompoundBody NewCompound;
        NewCompound.OwnerActor = OwnerActor;
        NewCompound.RootComponent = RootPrim;
        NewCompound.Body = BodyHandle;

        ActorCompounds[OwnerActor] = NewCompound;
        Compound = &ActorCompounds[OwnerActor];
    }

    FPhysicsShapeDesc ShapeDesc = BuildShapeDescFromComponent(Comp, Compound->RootComponent);
    FPhysicsShapeHandle ShapeHandle = AddShapeToBody(Compound->Body, Comp, ShapeDesc);
    if (!ShapeHandle.IsValid())
    {
        return;
    }

    Compound->Components.push_back(Comp);
    ComponentToBody[Comp] = Compound->Body;
    ComponentToShape[Comp] = ShapeHandle;

    FBodyInstance* Body = ResolveBody(Compound->Body);
    if (Body && Body->Actor)
    {
        FBodyCreationDesc MassDesc = BuildBodyDescFromComponent(Compound->RootComponent);
        FPhysXBodyBuilder::UpdateMassAndInertia(Body->Actor, MassDesc);
    }
}

void FPhysXPhysicsRuntime::UnregisterComponent(UPrimitiveComponent* Comp)
{
    if (!Comp)
    {
        return;
    }

    auto BodyIt = ComponentToBody.find(Comp);
    if (BodyIt == ComponentToBody.end())
    {
        return;
    }

    const FPhysicsBodyHandle BodyHandle = BodyIt->second;
    AActor* OwnerActor = Comp->GetOwner();

    DetachShapeForComponent(Comp);
    ComponentToBody.erase(Comp);

    FActorCompoundBody* Compound = FindCompoundByActor(OwnerActor);
    if (!Compound)
    {
        return;
    }

    Compound->Components.erase(
        std::remove(Compound->Components.begin(), Compound->Components.end(), Comp),
        Compound->Components.end()
    );

    if (Compound->Components.empty())
    {
        DestroyRigidBody(BodyHandle);
        ActorCompounds.erase(OwnerActor);
        return;
    }

    FBodyInstance* Body = ResolveBody(Compound->Body);
    if (Body && Body->Actor)
    {
        FBodyCreationDesc MassDesc = BuildBodyDescFromComponent(Compound->RootComponent);
        FPhysXBodyBuilder::UpdateMassAndInertia(Body->Actor, MassDesc);
    }
}

void FPhysXPhysicsRuntime::RebuildBody(UPrimitiveComponent* Comp)
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

    TArray<UPrimitiveComponent*> Components = Compound->Components;
    for (UPrimitiveComponent* C : Components)
    {
        if (IsValid(C))
        {
            UnregisterComponent(C);
        }
    }

    for (UPrimitiveComponent* C : Components)
    {
        if (IsValid(C) && C->IsCollisionEnabled())
        {
            RegisterComponent(C);
        }
    }
}

FPhysicsBodyHandle FPhysXPhysicsRuntime::FindBodyByComponent(UPrimitiveComponent* Comp) const
{
    auto It = ComponentToBody.find(Comp);
    return It != ComponentToBody.end() ? It->second : FPhysicsBodyHandle{};
}

FPhysicsShapeHandle FPhysXPhysicsRuntime::FindShapeByComponent(UPrimitiveComponent* Comp) const
{
    auto It = ComponentToShape.find(Comp);
    return It != ComponentToShape.end() ? It->second : FPhysicsShapeHandle{};
}

FPhysicsBodyHandle FPhysXPhysicsRuntime::CreateRigidBody(const FBodyCreationDesc& Desc)
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

void FPhysXPhysicsRuntime::DestroyRigidBody(FPhysicsBodyHandle BodyHandle)
{
    FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body)
    {
        return;
    }

    TArray<FPhysicsConstraintHandle> ConstraintsToDestroy = Body->Constraints;
    for (FPhysicsConstraintHandle Constraint : ConstraintsToDestroy)
    {
        DestroyConstraint(Constraint);
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
        if (Scene && Body->bRegisteredInScene)
        {
            Scene->removeActor(*Body->Actor);
        }

        Body->Actor->release();
        Body->Actor = nullptr;
    }

    FreeBody(BodyHandle);
}

FPhysicsConstraintHandle FPhysXPhysicsRuntime::CreateConstraint(
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

void FPhysXPhysicsRuntime::DestroyConstraint(FPhysicsConstraintHandle Handle)
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

FTransform FPhysXPhysicsRuntime::GetBodyTransform(FPhysicsBodyHandle BodyHandle) const
{
    const FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return FTransform();
    }

    return ToFTransform(Body->Actor->getGlobalPose());
}

void FPhysXPhysicsRuntime::SetBodyTransform(
    FPhysicsBodyHandle BodyHandle,
    const FTransform& Transform,
    EPhysicsTeleportMode
)
{
    FBodyInstance* Body = ResolveBody(BodyHandle);
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
    FBodyInstance* Body = ResolveBody(BodyHandle);
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
}

FVector FPhysXPhysicsRuntime::GetBodyLinearVelocity(FPhysicsBodyHandle BodyHandle) const
{
    const FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return FVector::ZeroVector;
    }

    PxRigidDynamic* Dynamic = Body->Actor->is<PxRigidDynamic>();
    if (!Dynamic)
    {
        return FVector::ZeroVector;
    }

    return ToFVector(Dynamic->getLinearVelocity());
}

FVector FPhysXPhysicsRuntime::GetBodyAngularVelocity(FPhysicsBodyHandle BodyHandle) const
{
    const FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return FVector::ZeroVector;
    }

    PxRigidDynamic* Dynamic = Body->Actor->is<PxRigidDynamic>();
    if (!Dynamic)
    {
        return FVector::ZeroVector;
    }

    return ToFVector(Dynamic->getAngularVelocity());
}

void FPhysXPhysicsRuntime::AddForce(FPhysicsBodyHandle BodyHandle, const FVector& Force)
{
    FBodyInstance* Body = ResolveBody(BodyHandle);
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
    FBodyInstance* Body = ResolveBody(BodyHandle);
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
    FBodyInstance* Body = ResolveBody(BodyHandle);
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
    FBodyInstance* Body = ResolveBody(BodyHandle);
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
    FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return;
    }

    FBodyCreationDesc Desc;
    Desc.Mass = Mass;
    Desc.CenterOfMassLocalOffset = GetCenterOfMass(BodyHandle);
    FPhysXBodyBuilder::UpdateMassAndInertia(Body->Actor, Desc);
}

float FPhysXPhysicsRuntime::GetMass(FPhysicsBodyHandle BodyHandle) const
{
    const FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return 1.0f;
    }

    PxRigidDynamic* Dynamic = Body->Actor->is<PxRigidDynamic>();
    if (!Dynamic)
    {
        return 1.0f;
    }

    return Dynamic->getMass();
}

void FPhysXPhysicsRuntime::SetCenterOfMass(FPhysicsBodyHandle BodyHandle, const FVector& LocalOffset)
{
    FBodyInstance* Body = ResolveBody(BodyHandle);
    if (!Body || !Body->Actor)
    {
        return;
    }

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

    PxRigidDynamic* Dynamic = Body->Actor->is<PxRigidDynamic>();
    if (!Dynamic)
    {
        return FVector::ZeroVector;
    }

    return ToFVector(Dynamic->getCMassLocalPose().p);
}

void FPhysXPhysicsRuntime::GatherDebugBodies(TArray<FPhysicsDebugBody>& OutBodies) const
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
        DebugBody.Body = Body.Handle;
        DebugBody.BodyWorldTransform = ToFTransform(Body.Actor->getGlobalPose());
        DebugBody.OwnerActor = Body.OwnerActor;
        DebugBody.OwnerComponent = Body.OwnerComponent;
        DebugBody.BoneName = Body.BoneName;

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
            if (!ShapeInstance)
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

            const FTransform ShapeLocal = ShapeInstance->Desc.LocalTransform;
            DebugShape.WorldTransform = ShapeLocal;
            DebugShape.WorldTransform.Location =
                DebugBody.BodyWorldTransform.Location +
                DebugBody.BodyWorldTransform.Rotation.RotateVector(ShapeLocal.Location);
            DebugShape.WorldTransform.Rotation =
                DebugBody.BodyWorldTransform.Rotation * ShapeLocal.Rotation;

            DebugBody.Shapes.push_back(DebugShape);
        }

        OutBodies.push_back(DebugBody);
    }
}

void FPhysXPhysicsRuntime::GatherDebugConstraints(TArray<FPhysicsDebugConstraint>& OutConstraints) const
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

        Debug.ParentFrameWorld = Constraint.ParentLocalFrame;
        Debug.ParentFrameWorld.Location =
            ParentWorld.Location +
            ParentWorld.Rotation.RotateVector(Constraint.ParentLocalFrame.Location);
        Debug.ParentFrameWorld.Rotation =
            ParentWorld.Rotation * Constraint.ParentLocalFrame.Rotation;

        Debug.ChildFrameWorld = Constraint.ChildLocalFrame;
        Debug.ChildFrameWorld.Location =
            ChildWorld.Location +
            ChildWorld.Rotation.RotateVector(Constraint.ChildLocalFrame.Location);
        Debug.ChildFrameWorld.Rotation =
            ChildWorld.Rotation * Constraint.ChildLocalFrame.Rotation;

        Debug.TwistLimitMinDegrees = Constraint.Limits.TwistLimitMinDegrees;
        Debug.TwistLimitMaxDegrees = Constraint.Limits.TwistLimitMaxDegrees;
        Debug.Swing1LimitDegrees = Constraint.Limits.Swing1LimitDegrees;
        Debug.Swing2LimitDegrees = Constraint.Limits.Swing2LimitDegrees;

        OutConstraints.push_back(Debug);
    }
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
    FillFilterDataFromComponent(Desc.FilterData, Comp);

    const bool bOwnerBodyIsDynamic =
        RootComponent &&
        (RootComponent->GetSimulatePhysics() || RootComponent->IsKinematic());

    Desc.bIsTrigger = ShouldBeTriggerShape(Comp, bOwnerBodyIsDynamic);

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

    ShapeInstance->OwnerBody = BodyHandle;
    ShapeInstance->SourceComponent = SourceComponent;
    ShapeInstance->Desc = Desc;
    ShapeInstance->Shape = Shape;

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
        Body->Actor->detachShape(*ShapeInstance->Shape);
        Body->Shapes.erase(
            std::remove(Body->Shapes.begin(), Body->Shapes.end(), ShapeHandle),
            Body->Shapes.end()
        );
    }

    if (ShapeInstance->SourceComponent)
    {
        ComponentToShape.erase(ShapeInstance->SourceComponent);
    }

    FreeShape(ShapeHandle);
}

void FPhysXPhysicsRuntime::DetachShapeForComponent(UPrimitiveComponent* Comp)
{
    auto It = ComponentToShape.find(Comp);
    if (It == ComponentToShape.end())
    {
        return;
    }

    DetachShape(It->second);
}

void FPhysXPhysicsRuntime::SyncEngineToPhysics(FBodyInstance& Body)
{
    if (!Body.Actor || !IsValid(Body.OwnerComponent))
    {
        return;
    }

    const FTransform Target = GetComponentWorldTransform(Body.OwnerComponent);
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
    if (!Body.Actor || !IsValid(Body.OwnerComponent))
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
    Body.CurrentTransform = ToFTransform(Pose);

    Body.OwnerComponent->SetWorldLocation(Body.CurrentTransform.Location);
    Body.OwnerComponent->SetRelativeRotation(Body.CurrentTransform.Rotation);
}

void FPhysXPhysicsRuntime::ApplyPendingCommands()
{
    TArray<FPhysicsCommand> Commands;
    CommandQueue.Drain(Commands);

    for (const FPhysicsCommand& Command : Commands)
    {
        switch (Command.Type)
        {
        case EPhysicsCommandType::DestroyBody:
            DestroyRigidBody(Command.Body);
            break;
        case EPhysicsCommandType::DestroyConstraint:
            DestroyConstraint(Command.Constraint);
            break;
        case EPhysicsCommandType::SetTransform:
        case EPhysicsCommandType::SetKinematicTarget:
            SetBodyTransform(Command.Body, Command.TransformValue, Command.TeleportMode);
            break;
        case EPhysicsCommandType::AddForce:
            AddForce(Command.Body, Command.VectorValue);
            break;
        case EPhysicsCommandType::AddForceAtLocation:
            AddForceAtLocation(Command.Body, Command.VectorValue, Command.VectorValue2);
            break;
        case EPhysicsCommandType::AddTorque:
            AddTorque(Command.Body, Command.VectorValue);
            break;
        case EPhysicsCommandType::AddImpulse:
            AddImpulse(Command.Body, Command.VectorValue);
            break;
        case EPhysicsCommandType::SetLinearVelocity:
            SetBodyVelocity(Command.Body, Command.VectorValue, GetBodyAngularVelocity(Command.Body));
            break;
        case EPhysicsCommandType::SetAngularVelocity:
            SetBodyVelocity(Command.Body, GetBodyLinearVelocity(Command.Body), Command.VectorValue);
            break;
        case EPhysicsCommandType::SetMass:
            SetMass(Command.Body, Command.FloatValue);
            break;
        case EPhysicsCommandType::SetCenterOfMass:
            SetCenterOfMass(Command.Body, Command.VectorValue);
            break;
        default:
            break;
        }
    }
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

    auto It = ActorCompounds.find(Actor);
    return It != ActorCompounds.end() ? &It->second : nullptr;
}

const FActorCompoundBody* FPhysXPhysicsRuntime::FindCompoundByActor(AActor* Actor) const
{
    if (!IsValid(Actor))
    {
        return nullptr;
    }

    auto It = ActorCompounds.find(Actor);
    return It != ActorCompounds.end() ? &It->second : nullptr;
}
