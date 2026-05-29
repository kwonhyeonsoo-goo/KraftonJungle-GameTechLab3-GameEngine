#pragma once

#include "Physics/PhysicsTypes.h"

struct FPhysicsDebugShape
{
    FPhysicsShapeHandle Shape;

    EPhysicsShapeType Type = EPhysicsShapeType::Box;

    FTransform WorldTransform;

    FVector BoxHalfExtent = FVector::ZeroVector;

    float SphereRadius = 0.0f;

    float CapsuleRadius     = 0.0f;
    float CapsuleHalfHeight = 0.0f;
};

struct FPhysicsDebugBody
{
    FPhysicsBodyHandle Body;

    FTransform BodyWorldTransform;

    bool bIsStatic    = false;
    bool bIsDynamic   = false;
    bool bIsKinematic = false;
    bool bIsSleeping  = false;

    AActor*              OwnerActor     = nullptr;
    UPrimitiveComponent* OwnerComponent = nullptr;
    FName                BoneName       = FName::None;

    TArray<FPhysicsDebugShape> Shapes;
};

struct FPhysicsDebugConstraint
{
    FPhysicsConstraintHandle Constraint;

    FPhysicsBodyHandle ParentBody;
    FPhysicsBodyHandle ChildBody;

    FTransform ParentFrameWorld;
    FTransform ChildFrameWorld;

    float TwistLimitMinDegrees = 0.0f;
    float TwistLimitMaxDegrees = 0.0f;
    float Swing1LimitDegrees   = 0.0f;
    float Swing2LimitDegrees   = 0.0f;
};