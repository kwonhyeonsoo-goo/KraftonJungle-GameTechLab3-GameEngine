#pragma once

#include "Physics/PhysicsBodyInstance.h"
#include "Physics/PhysicsCommandQueue.h"
#include "Physics/PhysicsConstraintInstance.h"
#include "Physics/PhysicsDebugTypes.h"
#include "Physics/PhysicsRuntime.h"
#include "Physics/PhysicsShapeInstance.h"
#include "Physics/PhysicsStats.h"

#include <memory>
#include <mutex>

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

    // 외부 소비자(C/D)는 이 세 함수로만 디버그 데이터를 얻는다 — 전부 step 직후 publish 된
    // 스냅샷의 복사본을 lock 아래에서 반환한다 (live PhysX 직접 접근 없음).
    void GatherDebugBodies(TArray<FPhysicsDebugBody>& OutBodies) const override;
    void GatherDebugConstraints(TArray<FPhysicsDebugConstraint>& OutConstraints) const override;
    void GetDebugSnapshot(FPhysicsDebugSnapshot& OutSnapshot) const;

    const FPhysicsStats& GetStats() const override
    {
        return Stats;
    }

private:
    FPhysicsBodyHandle       AllocateBody();
    FPhysicsShapeHandle      AllocateShape();
    FPhysicsConstraintHandle AllocateConstraint();
    FPhysicsBodyHandle       CreateRigidBody_Internal(const FBodyCreationDesc& Desc);
    void                     DestroyRigidBody_Internal(FPhysicsBodyHandle Body);
    FPhysicsConstraintHandle CreateConstraint_Internal(
        FPhysicsBodyHandle             Parent,
        FPhysicsBodyHandle             Child,
        const FConstraintCreationDesc& Desc
    );
    void DestroyConstraint_Internal(FPhysicsConstraintHandle Constraint);


    FBodyInstance*       ResolveBody(FPhysicsBodyHandle Handle);
    const FBodyInstance* ResolveBody(FPhysicsBodyHandle Handle) const;

    // generation + State(Alive) 까지 검사. PendingDestroy/Destroyed body 의 stale access 차단.
    FBodyInstance*       ResolveAliveBody(FPhysicsBodyHandle Handle);
    const FBodyInstance* ResolveAliveBody(FPhysicsBodyHandle Handle) const;

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

    // Public mutator 들은 command 만 큐에 넣는다. 실제 PhysX 변경은 physics step 의
    // ApplyPendingCommands 가 아래 ApplyXxx_Internal 들을 호출할 때만 일어난다.
    void EnqueueCommand(const FPhysicsCommand& Command);

    void ApplyAddForce_Internal(FPhysicsBodyHandle Body, const FVector& Force);
    void ApplyAddForceAtLocation_Internal(FPhysicsBodyHandle Body, const FVector& Force, const FVector& WorldLocation);
    void ApplyAddTorque_Internal(FPhysicsBodyHandle Body, const FVector& Torque);
    void ApplyAddImpulse_Internal(FPhysicsBodyHandle Body, const FVector& Impulse);
    void ApplySetBodyVelocity_Internal(FPhysicsBodyHandle Body, const FVector& LinearVelocity, const FVector& AngularVelocity);
    void ApplySetBodyTransform_Internal(FPhysicsBodyHandle Body, const FTransform& Transform, EPhysicsTeleportMode TeleportMode);
    void ApplySetMass_Internal(FPhysicsBodyHandle Body, float Mass);
    void ApplySetCenterOfMass_Internal(FPhysicsBodyHandle Body, const FVector& LocalOffset);
    void ApplySetLinearLock_Internal(FPhysicsBodyHandle Body, bool bX, bool bY, bool bZ);
    void ApplySetAngularLock_Internal(FPhysicsBodyHandle Body, bool bX, bool bY, bool bZ);

    // 적용한 command 수를 반환 (Stat 용).
    int32 ApplyPendingCommands();
    void  UpdateStats();

    // 스냅샷 빌더 — step 내부(read 시점)에서만 live PhysX 를 읽어 스냅샷을 구성한다.
    void BuildDebugBodies_Internal(TArray<FPhysicsDebugBody>& OutBodies) const;
    void BuildDebugConstraints_Internal(TArray<FPhysicsDebugConstraint>& OutConstraints) const;
    void BuildDebugSnapshot_Internal();

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

    // 단조 증가 physics step 카운터 — body CreatedStep/DestroyedStep, snapshot StepIndex 용.
    uint64 StepIndex = 0;

    // Debug/Stat 스냅샷 — step 직후 publish, 외부는 lock 아래에서 복사본만 읽는다.
    bool                  bDebugSnapshotEnabled = true;
    mutable std::mutex    DebugSnapshotMutex;
    FPhysicsDebugSnapshot DebugSnapshot;
};