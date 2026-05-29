#pragma once

#include "Physics/PhysicsTypes.h"

struct FPhysicsDebugBody;
struct FPhysicsDebugConstraint;
struct FPhysicsStats;

class IPhysicsRuntime
{
public:
    virtual ~IPhysicsRuntime() = default;

    virtual FPhysicsBodyHandle CreateRigidBody(const FBodyCreationDesc& Desc) = 0;
    virtual void               DestroyRigidBody(FPhysicsBodyHandle Body) = 0;

    virtual FPhysicsConstraintHandle CreateConstraint(
        FPhysicsBodyHandle             Parent,
        FPhysicsBodyHandle             Child,
        const FConstraintCreationDesc& Desc
    ) = 0;

    virtual void DestroyConstraint(FPhysicsConstraintHandle Constraint) = 0;

    virtual FTransform GetBodyTransform(FPhysicsBodyHandle Body) const = 0;

    virtual void SetBodyTransform(
        FPhysicsBodyHandle   Body,
        const FTransform&    Transform,
        EPhysicsTeleportMode TeleportMode
    ) = 0;

    virtual void SetBodyVelocity(
        FPhysicsBodyHandle Body,
        const FVector&     LinearVelocity,
        const FVector&     AngularVelocity
    ) = 0;

    virtual FVector GetBodyLinearVelocity(FPhysicsBodyHandle Body) const = 0;
    virtual FVector GetBodyAngularVelocity(FPhysicsBodyHandle Body) const = 0;

    virtual void AddForce(FPhysicsBodyHandle Body, const FVector& Force) = 0;
    virtual void AddForceAtLocation(
        FPhysicsBodyHandle Body,
        const FVector&     Force,
        const FVector&     WorldLocation
    ) = 0;
    virtual void AddTorque(FPhysicsBodyHandle Body, const FVector& Torque) = 0;
    virtual void AddImpulse(FPhysicsBodyHandle Body, const FVector& Impulse) = 0;

    virtual void  SetMass(FPhysicsBodyHandle Body, float Mass) = 0;
    virtual float GetMass(FPhysicsBodyHandle Body) const = 0;

    virtual void    SetCenterOfMass(FPhysicsBodyHandle Body, const FVector& LocalOffset) = 0;
    virtual FVector GetCenterOfMass(FPhysicsBodyHandle Body) const = 0;

    virtual void SetLinearLock(FPhysicsBodyHandle Body, bool bX, bool bY, bool bZ) = 0;
    virtual void SetAngularLock(FPhysicsBodyHandle Body, bool bX, bool bY, bool bZ) = 0;

    virtual void GatherDebugBodies(TArray<FPhysicsDebugBody>& OutBodies) const = 0;
    virtual void GatherDebugConstraints(TArray<FPhysicsDebugConstraint>& OutConstraints) const = 0;

    virtual const FPhysicsStats& GetStats() const = 0;
};