#pragma once

#include "Physics/PhysicsTypes.h"

namespace physx
{
    class PxRigidActor;
}

struct FBodyInstance
{
    FPhysicsBodyHandle Handle;

    AActor*              OwnerActor     = nullptr;
    UPrimitiveComponent* OwnerComponent = nullptr;

    FName BoneName = FName::None;

    physx::PxRigidActor* Actor = nullptr;

    TArray<FPhysicsShapeHandle>      Shapes;
    TArray<FPhysicsConstraintHandle> Constraints;

    EPhysicsBodyType BodyType = EPhysicsBodyType::Static;
    EPhysicsSyncMode SyncMode = EPhysicsSyncMode::EngineToPhysics;

    FTransform PreviousTransform;
    FTransform CurrentTransform;

    bool bRegisteredInScene     = false;
    bool bGenerateHitEvents     = false;
    bool bGenerateOverlapEvents = false;

    bool IsManualSync() const
    {
        return SyncMode == EPhysicsSyncMode::Manual;
    }

    bool ShouldPullTransformToEngine() const
    {
        return SyncMode == EPhysicsSyncMode::PhysicsToEngine;
    }

    bool ShouldPushTransformToPhysics() const
    {
        return SyncMode == EPhysicsSyncMode::EngineToPhysics;
    }

    bool IsKinematicTargetDriven() const
    {
        return SyncMode == EPhysicsSyncMode::KinematicTarget;
    }
};