#pragma once

#include "Physics/PhysicsTypes.h"

enum class EPhysicsCommandType : uint8
{
    DestroyBody,
    DestroyConstraint,

    SetTransform,
    SetKinematicTarget,

    AddForce,
    AddForceAtLocation,
    AddTorque,
    AddImpulse,

    SetLinearVelocity,
    SetAngularVelocity,

    SetMass,
    SetCenterOfMass,
};

struct FPhysicsCommand
{
    EPhysicsCommandType Type = EPhysicsCommandType::DestroyBody;

    FPhysicsBodyHandle       Body;
    FPhysicsConstraintHandle Constraint;

    FTransform TransformValue;
    FVector    VectorValue  = FVector::ZeroVector;
    FVector    VectorValue2 = FVector::ZeroVector;

    float FloatValue = 0.0f;

    EPhysicsTeleportMode TeleportMode = EPhysicsTeleportMode::None;
};

class FPhysicsCommandQueue
{
public:
    void Enqueue(const FPhysicsCommand& Command)
    {
        Commands.push_back(Command);
    }

    bool IsEmpty() const
    {
        return Commands.empty();
    }

    void Drain(TArray<FPhysicsCommand>& OutCommands)
    {
        OutCommands.insert(OutCommands.end(), Commands.begin(), Commands.end());
        Commands.clear();
    }

    void Clear()
    {
        Commands.clear();
    }

private:
    TArray<FPhysicsCommand> Commands;
};