#pragma once

#include "Core/Types/CollisionTypes.h"
#include "Core/Types/CoreTypes.h"
#include "Math/Transform.h"
#include "Math/Vector.h"
#include "Physics/PhysicsTypes.h"

#include <cfloat>

struct FPhysXVehicleHandle
{
    uint32 Index      = UINT32_MAX;
    uint32 Generation = 0;

    bool IsValid() const
    {
        return Index != UINT32_MAX && Generation != 0;
    }

    bool operator==(const FPhysXVehicleHandle& Other) const
    {
        return Index == Other.Index && Generation == Other.Generation;
    }

    bool operator!=(const FPhysXVehicleHandle& Other) const
    {
        return !(*this == Other);
    }
};

inline uint64 MakePhysXVehicleHandleKey(FPhysXVehicleHandle Vehicle)
{
    return (static_cast<uint64>(Vehicle.Index) << 32) | static_cast<uint64>(Vehicle.Generation);
}

struct FPhysXVehicleInputState
{
    float Throttle  = 0.0f;
    float Brake     = 0.0f;
    float Steer     = 0.0f;
    float Handbrake = 0.0f;
};

enum class EPhysXVehicleWheelIndex : uint8
{
    FrontLeft  = 0,
    FrontRight = 1,
    RearLeft   = 2,
    RearRight  = 3,
    Count      = 4
};

struct FPhysXVehicleWheelDesc
{
    FVector LocalPosition = FVector::ZeroVector;

    float Radius = 0.35f;
    float Width  = 0.25f;
    float Mass   = 20.0f;
    float MOI    = 1.225f; // 0.5 * Mass * Radius^2 근사.

    float MaxBrakeTorque     = 1500.0f;
    float MaxHandbrakeTorque = 0.0f;
    float MaxSteerRadians    = 0.0f;

    float SuspensionMaxCompression   = 0.30f;
    float SuspensionMaxDroop         = 0.10f;
    float SuspensionSpringStrength   = 35000.0f;
    float SuspensionSpringDamperRate = 4500.0f;

    uint32 TireType = 0;
};

struct FPhysXVehicleDesc
{
    FPhysicsObjectKey Owner;

    FPhysXVehicleHandle ReservedVehicle;
    FTransform          WorldTransform;

    FVector ChassisHalfExtents = FVector(1.25f, 0.6f, 0.35f);
    float   ChassisMass        = 1200.0f;
    FVector ChassisCMOffset    = FVector(0.0f, 0.0f, -0.25f);

    FPhysXVehicleWheelDesc Wheels[4];

    float EnginePeakTorque = 500.0f;
    float EngineMaxOmega   = 600.0f;
    float ClutchStrength   = 10.0f;

    float TireFriction = 1.5f;

    static FPhysXVehicleDesc MakeDefault4W()
    {
        FPhysXVehicleDesc Desc;

        Desc.Wheels[static_cast<int32>(EPhysXVehicleWheelIndex::FrontLeft)].LocalPosition  = FVector(1.1f, -0.75f, -0.35f);
        Desc.Wheels[static_cast<int32>(EPhysXVehicleWheelIndex::FrontRight)].LocalPosition = FVector(1.1f, 0.75f, -0.35f);
        Desc.Wheels[static_cast<int32>(EPhysXVehicleWheelIndex::RearLeft)].LocalPosition   = FVector(-1.1f, -0.75f, -0.35f);
        Desc.Wheels[static_cast<int32>(EPhysXVehicleWheelIndex::RearRight)].LocalPosition  = FVector(-1.1f, 0.75f, -0.35f);

        Desc.Wheels[static_cast<int32>(EPhysXVehicleWheelIndex::FrontLeft)].MaxSteerRadians    = 0.6f;
        Desc.Wheels[static_cast<int32>(EPhysXVehicleWheelIndex::FrontRight)].MaxSteerRadians   = 0.6f;
        Desc.Wheels[static_cast<int32>(EPhysXVehicleWheelIndex::RearLeft)].MaxHandbrakeTorque  = 4000.0f;
        Desc.Wheels[static_cast<int32>(EPhysXVehicleWheelIndex::RearRight)].MaxHandbrakeTorque = 4000.0f;

        return Desc;
    }
};

struct FPhysXVehicleWheelSnapshot
{
    FTransform WorldTransform;

    float SteerAngle       = 0.0f;
    float RotationAngle    = 0.0f;
    float SuspensionJounce = 0.0f;

    bool bInAir = true;

    FVector ContactPoint  = FVector::ZeroVector;
    FVector ContactNormal = FVector::UpVector;
};

struct FPhysXVehicleSnapshot
{
    FPhysXVehicleHandle Vehicle;

    uint32 OwnerActorId             = 0;
    uint32 OwnerComponentId         = 0;
    uint32 OwnerComponentGeneration = 0;

    FTransform ChassisWorldTransform;

    FVector LinearVelocity  = FVector::ZeroVector;
    FVector AngularVelocity = FVector::ZeroVector;

    FPhysXVehicleWheelSnapshot Wheels[4];
};