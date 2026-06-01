#pragma once

#include "Component/Movement/MovementComponent.h"
#include "Object/FName.h"
#include "Object/Ptr/ObjectPtr.h"
#include "Physics/Vehicle/VehicleTypes.h"

#include "Source/Engine/Component/Movement/WheeledVehicleMovementComponent.generated.h"

struct FPhysicsWorldSnapshot;

USTRUCT()
struct FVehicleWheelSetup
{
    GENERATED_BODY()

    UPROPERTY(Edit, Save, Category="Wheel", DisplayName="Wheel Name")
    FName WheelName = FName::None;

    UPROPERTY(Edit, Save, Category="Wheel", DisplayName="Bone Name")
    FName BoneName = FName::None;

    UPROPERTY(Edit, Save, Category="Wheel", DisplayName="Visual Component Name")
    FName VisualComponentName = FName::None;

    UPROPERTY(Edit, Save, Category="Wheel", DisplayName="Local Position", Type=Vec3, Speed=0.01f)
    FVector LocalPosition = FVector::ZeroVector;

    UPROPERTY(Edit, Save, Category="Wheel", DisplayName="Radius", Speed=0.01f)
    float Radius = 0.35f;
    UPROPERTY(Edit, Save, Category="Wheel", DisplayName="Width", Speed=0.01f)
    float Width = 0.25f;
    UPROPERTY(Edit, Save, Category="Wheel", DisplayName="Mass", Speed=0.1f)
    float Mass = 20.0f;

    UPROPERTY(Edit, Save, Category="Wheel", DisplayName="Spring Strength", Speed=100.0f)
    float SuspensionSpringStrength = 35000.0f;
    UPROPERTY(Edit, Save, Category="Wheel", DisplayName="Spring Damper Rate", Speed=10.0f)
    float SuspensionSpringDamperRate = 4500.0f;
    UPROPERTY(Edit, Save, Category="Wheel", DisplayName="Suspension Max Compression", Speed=0.01f)
    float SuspensionMaxCompression = 0.30f;
    UPROPERTY(Edit, Save, Category="Wheel", DisplayName="Suspension Max Droop", Speed=0.01f)
    float SuspensionMaxDroop = 0.10f;

    UPROPERTY(Edit, Save, Category="Wheel", DisplayName="Max Brake Torque", Speed=10.0f)
    float MaxBrakeTorque = 1500.0f;
    UPROPERTY(Edit, Save, Category="Wheel", DisplayName="Max Handbrake Torque", Speed=10.0f)
    float MaxHandbrakeTorque = 0.0f;
    UPROPERTY(Edit, Save, Category="Wheel", DisplayName="Max Steer Angle (deg)", Speed=0.5f)
    float MaxSteerAngleDegrees = 0.0f;
};

/**
 * Backend-neutral wheeled vehicle movement component.
 *
 * This component owns gameplay input and vehicle simulation state only. It builds an
 * FVehicleDesc and talks to IPhysicsScene; it does not know PhysX public types and it
 * does not guess or directly drive wheel visuals. Wheel visuals are handled by
 * UVehicleWheelPoseComponent using WheelSetups + LastSnapshot.
 */
UCLASS()
class UWheeledVehicleMovementComponent : public UMovementComponent
{
public:
    GENERATED_BODY()

    UWheeledVehicleMovementComponent() = default;
    ~UWheeledVehicleMovementComponent() override = default;

    void BeginPlay() override;
    void EndPlay() override;
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;

    UFUNCTION(Callable, Category="Vehicle|Input")
    void SetThrottleInput(float InThrottle);
    UFUNCTION(Callable, Category="Vehicle|Input")
    void SetBrakeInput(float InBrake);
    UFUNCTION(Callable, Category="Vehicle|Input")
    void SetSteeringInput(float InSteering);
    UFUNCTION(Callable, Category="Vehicle|Input")
    void SetHandbrakeInput(float InHandbrake);

    UFUNCTION(Callable, Category="Vehicle")
    void ResetVehicle();
    UFUNCTION(Pure, Category="Vehicle")
    float GetForwardSpeed() const;

    bool IsVehicleCreated() const { return VehicleHandle.IsValid(); }
    FVehicleHandle GetVehicleHandle() const { return VehicleHandle; }

    const FVehicleSnapshot* GetLastVehicleSnapshot() const { return bHasLastSnapshot ? &LastSnapshot : nullptr; }
    const TArray<FVehicleWheelSetup>& GetWheelSetups() const { return WheelSetups; }
    const FVehicleWheelSetup* FindWheelSetup(const FName& WheelName) const;

protected:
    FVehicleDesc BuildVehicleDesc() const;
    void EnsureDefaultWheelSetups();
    void RegisterPhysicsSnapshotReceiver();
    void UnregisterPhysicsSnapshotReceiver();
    void ConsumeVehicleSnapshot(const FPhysicsWorldSnapshot& Snapshot);
    void ApplyVehicleSnapshot(const FVehicleSnapshot& Snapshot);

protected:
    UPROPERTY(Edit, Save, Category="Vehicle|Chassis", DisplayName="Chassis Half Extents", Type=Vec3, Speed=0.05f)
    FVector ChassisHalfExtents = FVector(1.25f, 0.6f, 0.35f);
    UPROPERTY(Edit, Save, Category="Vehicle|Chassis", DisplayName="Chassis Mass", Speed=1.0f)
    float ChassisMass = 1200.0f;
    UPROPERTY(Edit, Save, Category="Vehicle|Chassis", DisplayName="Center Of Mass Offset", Type=Vec3, Speed=0.01f)
    FVector ChassisCenterOfMassOffset = FVector(0.0f, 0.0f, -0.25f);

    UPROPERTY(Edit, Save, Category="Vehicle|Engine", DisplayName="Engine Peak Torque", Speed=1.0f)
    float EnginePeakTorque = 500.0f;
    UPROPERTY(Edit, Save, Category="Vehicle|Engine", DisplayName="Engine Max Omega", Speed=1.0f)
    float EngineMaxOmega = 600.0f;
    UPROPERTY(Edit, Save, Category="Vehicle|Engine", DisplayName="Clutch Strength", Speed=0.1f)
    float ClutchStrength = 10.0f;

    UPROPERTY(Edit, Save, Category="Vehicle|Tire", DisplayName="Tire Friction", Speed=0.05f)
    float TireFriction = 1.5f;

    UPROPERTY(Edit, Save, Category="Vehicle|Wheel", DisplayName="Wheel Setups", Type=Array, Struct=FVehicleWheelSetup)
    TArray<FVehicleWheelSetup> WheelSetups;

    FVehicleHandle     VehicleHandle;
    FVehicleInputState CurrentInput;
    FVehicleSnapshot   LastSnapshot;
    bool               bHasLastSnapshot = false;
    uint64             PhysicsSnapshotReceiverHandle = 0;
};
