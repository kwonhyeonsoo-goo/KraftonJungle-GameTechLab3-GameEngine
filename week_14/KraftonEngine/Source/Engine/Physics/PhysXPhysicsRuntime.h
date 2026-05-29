#pragma once

#include "Physics/PhysicsBodyInstance.h"
#include "Physics/PhysicsCommandQueue.h"
#include "Physics/PhysicsConstraintInstance.h"
#include "Physics/PhysicsDebugTypes.h"
#include "Physics/PhysicsRuntime.h"
#include "Physics/PhysicsShapeInstance.h"
#include "Physics/PhysicsStats.h"

#include <memory>

class UWorld;

namespace physx
{
    class PxPhysics;
    class PxScene;
    class PxDefaultCpuDispatcher;
    class PxMaterial;
    class PxRigidActor;
    class PxShape;
}

struct FActorCompoundBody
{
    AActor*              OwnerActor    = nullptr;
    UPrimitiveComponent* RootComponent = nullptr;

    FPhysicsBodyHandle Body;

    TArray<UPrimitiveComponent*> Components;
};

class FPhysXPhysicsRuntime : public IPhysicsRuntime
{
public:
    FPhysXPhysicsRuntime()           = default;
    ~FPhysXPhysicsRuntime() override = default;

    void Initialize(
        UWorld*            InWorld,
        physx::PxPhysics*  InPhysics,
        physx::PxScene*    InScene,
        physx::PxMaterial* InDefaultMaterial
    );

    void Shutdown();

    void Tick(float DeltaTime);

    void RegisterComponent(UPrimitiveComponent* Comp);
    void UnregisterComponent(UPrimitiveComponent* Comp);
    void RebuildBody(UPrimitiveComponent* Comp);

    FPhysicsBodyHandle  FindBodyByComponent(UPrimitiveComponent* Comp) const;
    FPhysicsShapeHandle FindShapeByComponent(UPrimitiveComponent* Comp) const;

    FPhysicsBodyHandle CreateRigidBody(const FBodyCreationDesc& Desc) override;
    void               DestroyRigidBody(FPhysicsBodyHandle Body) override;

    FPhysicsConstraintHandle CreateConstraint(
        FPhysicsBodyHandle             Parent,
        FPhysicsBodyHandle             Child,
        const FConstraintCreationDesc& Desc
    ) override;

    void DestroyConstraint(FPhysicsConstraintHandle Constraint) override;

    FTransform GetBodyTransform(FPhysicsBodyHandle Body) const override;

    void SetBodyTransform(
        FPhysicsBodyHandle   Body,
        const FTransform&    Transform,
        EPhysicsTeleportMode TeleportMode
    ) override;

    void SetBodyVelocity(
        FPhysicsBodyHandle Body,
        const FVector&     LinearVelocity,
        const FVector&     AngularVelocity
    ) override;

    FVector GetBodyLinearVelocity(FPhysicsBodyHandle Body) const override;
    FVector GetBodyAngularVelocity(FPhysicsBodyHandle Body) const override;

    void AddForce(FPhysicsBodyHandle Body, const FVector& Force) override;
    void AddForceAtLocation(
        FPhysicsBodyHandle Body,
        const FVector&     Force,
        const FVector&     WorldLocation
    ) override;
    void AddTorque(FPhysicsBodyHandle Body, const FVector& Torque) override;
    void AddImpulse(FPhysicsBodyHandle Body, const FVector& Impulse) override;

    void  SetMass(FPhysicsBodyHandle Body, float Mass) override;
    float GetMass(FPhysicsBodyHandle Body) const override;

    void    SetCenterOfMass(FPhysicsBodyHandle Body, const FVector& LocalOffset) override;
    FVector GetCenterOfMass(FPhysicsBodyHandle Body) const override;

    void SetLinearLock(FPhysicsBodyHandle Body, bool bX, bool bY, bool bZ) override;
    void SetAngularLock(FPhysicsBodyHandle Body, bool bX, bool bY, bool bZ) override;

    void GatherDebugBodies(TArray<FPhysicsDebugBody>& OutBodies) const override;
    void GatherDebugConstraints(TArray<FPhysicsDebugConstraint>& OutConstraints) const override;

    const FPhysicsStats& GetStats() const override
    {
        return Stats;
    }

private:
    FPhysicsBodyHandle       AllocateBody();
    FPhysicsShapeHandle      AllocateShape();
    FPhysicsConstraintHandle AllocateConstraint();

    FBodyInstance*       ResolveBody(FPhysicsBodyHandle Handle);
    const FBodyInstance* ResolveBody(FPhysicsBodyHandle Handle) const;

    FShapeInstance*       ResolveShape(FPhysicsShapeHandle Handle);
    const FShapeInstance* ResolveShape(FPhysicsShapeHandle Handle) const;

    FConstraintInstance*       ResolveConstraint(FPhysicsConstraintHandle Handle);
    const FConstraintInstance* ResolveConstraint(FPhysicsConstraintHandle Handle) const;

    void FreeBody(FPhysicsBodyHandle Handle);
    void FreeShape(FPhysicsShapeHandle Handle);
    void FreeConstraint(FPhysicsConstraintHandle Handle);

    FBodyCreationDesc BuildBodyDescFromComponent(UPrimitiveComponent* Comp) const;
    FPhysicsShapeDesc BuildShapeDescFromComponent(
        UPrimitiveComponent* Comp,
        UPrimitiveComponent* RootComponent
    ) const;

    FPhysicsShapeHandle AddShapeToBody(
        FPhysicsBodyHandle       BodyHandle,
        UPrimitiveComponent*     SourceComponent,
        const FPhysicsShapeDesc& Desc
    );

    void DetachShape(FPhysicsShapeHandle ShapeHandle);
    void DetachShapeForComponent(UPrimitiveComponent* Comp);

    void SyncEngineToPhysics(FBodyInstance& Body);
    void SyncPhysicsToEngine(FBodyInstance& Body);

    void ApplyPendingCommands();
    void UpdateStats();

    FActorCompoundBody*       FindCompoundByActor(AActor* Actor);
    const FActorCompoundBody* FindCompoundByActor(AActor* Actor) const;

private:
    UWorld* World = nullptr;

    physx::PxPhysics*  Physics         = nullptr;
    physx::PxScene*    Scene           = nullptr;
    physx::PxMaterial* DefaultMaterial = nullptr;

    TArray<std::unique_ptr<FBodyInstance>> Bodies;
    TArray<uint32>                         BodyGenerations;

    TArray<std::unique_ptr<FShapeInstance>> Shapes;
    TArray<uint32>                          ShapeGenerations;

    TArray<std::unique_ptr<FConstraintInstance>> Constraints;
    TArray<uint32>                               ConstraintGenerations;

    TMap<AActor*, FActorCompoundBody>               ActorCompounds;
    TMap<UPrimitiveComponent*, FPhysicsBodyHandle>  ComponentToBody;
    TMap<UPrimitiveComponent*, FPhysicsShapeHandle> ComponentToShape;

    FPhysicsCommandQueue CommandQueue;

    FPhysicsStats Stats;

    float Accumulator = 0.0f;
    float FixedDt     = 1.0f / 60.0f;
    float MaxFrameDt  = 0.1f;
    int32 MaxSubsteps = 4;
};