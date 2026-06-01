#include "Component/Movement/WheeledVehicleMovementComponent.h"

#include "Component/SceneComponent.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"
#include "Physics/IPhysicsScene.h"
#include "Physics/PhysicsWorldSnapshot.h"

#include <algorithm>
#include <cmath>
#include "Object/Ptr/WeakObjectPtr.h"

namespace
{
    constexpr float VehicleDegToRad = 3.14159265358979323846f / 180.0f;

    IPhysicsScene* GetPhysicsScene(const UActorComponent* Component)
    {
        UWorld* World = Component ? Component->GetWorld() : nullptr;
        return World ? World->GetPhysicsScene() : nullptr;
    }

    FVehicleWheelSetup MakeDefaultWheelSetup(
        const char* WheelName,
        const char* BoneName,
        const FVector& LocalPosition,
        float MaxSteerAngleDegrees,
        float MaxHandbrakeTorque)
    {
        FVehicleWheelSetup Setup;
        Setup.WheelName = FName(WheelName);
        Setup.BoneName = FName(BoneName);
        Setup.VisualComponentName = FName(WheelName);
        Setup.LocalPosition = LocalPosition;
        Setup.MaxSteerAngleDegrees = MaxSteerAngleDegrees;
        Setup.MaxHandbrakeTorque = MaxHandbrakeTorque;
        return Setup;
    }
}

void UWheeledVehicleMovementComponent::BeginPlay()
{
    UMovementComponent::BeginPlay();

    EnsureDefaultWheelSetups();

    if (VehicleHandle.IsValid())
    {
        RegisterPhysicsSnapshotReceiver();
        return;
    }

    IPhysicsScene* PhysicsScene = GetPhysicsScene(this);
    if (!PhysicsScene || !GetUpdatedComponent())
    {
        return;
    }

    VehicleHandle = PhysicsScene->CreateVehicle(BuildVehicleDesc());
    RegisterPhysicsSnapshotReceiver();
}

void UWheeledVehicleMovementComponent::EndPlay()
{
    UnregisterPhysicsSnapshotReceiver();

    if (VehicleHandle.IsValid())
    {
        if (IPhysicsScene* PhysicsScene = GetPhysicsScene(this))
        {
            PhysicsScene->DestroyVehicle(VehicleHandle);
        }
        VehicleHandle = FVehicleHandle {};
    }

    bHasLastSnapshot = false;
    UMovementComponent::EndPlay();
}

void UWheeledVehicleMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
{
    UMovementComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!VehicleHandle.IsValid())
    {
        return;
    }

    if (IPhysicsScene* PhysicsScene = GetPhysicsScene(this))
    {
        PhysicsScene->SetVehicleInput(VehicleHandle, CurrentInput);
    }
}

void UWheeledVehicleMovementComponent::SetThrottleInput(float InThrottle)
{
    CurrentInput.Throttle = std::clamp(InThrottle, 0.0f, 1.0f);
}

void UWheeledVehicleMovementComponent::SetBrakeInput(float InBrake)
{
    CurrentInput.Brake = std::clamp(InBrake, 0.0f, 1.0f);
}

void UWheeledVehicleMovementComponent::SetSteeringInput(float InSteering)
{
    CurrentInput.Steering = std::clamp(InSteering, -1.0f, 1.0f);
}

void UWheeledVehicleMovementComponent::SetHandbrakeInput(float InHandbrake)
{
    CurrentInput.Handbrake = std::clamp(InHandbrake, 0.0f, 1.0f);
}

float UWheeledVehicleMovementComponent::GetForwardSpeed() const
{
    if (bHasLastSnapshot)
    {
        return LastSnapshot.LinearVelocity.Dot(LastSnapshot.ChassisWorldTransform.Rotation.GetForwardVector());
    }

    if (const USceneComponent* Chassis = GetUpdatedComponent())
    {
        return Chassis->GetForwardVector().Dot(FVector::ZeroVector);
    }

    return 0.0f;
}

void UWheeledVehicleMovementComponent::ResetVehicle()
{
    if (!VehicleHandle.IsValid())
    {
        return;
    }

    IPhysicsScene*   PhysicsScene = GetPhysicsScene(this);
    USceneComponent* Chassis      = GetUpdatedComponent();
    if (!PhysicsScene || !Chassis)
    {
        return;
    }

    const FTransform ChassisWorld(Chassis->GetWorldLocation(), Chassis->GetWorldRotation(), FVector(1.0f, 1.0f, 1.0f));
    PhysicsScene->ResetVehicle(VehicleHandle, ChassisWorld);
}

const FVehicleWheelSetup* UWheeledVehicleMovementComponent::FindWheelSetup(const FName& WheelName) const
{
    for (const FVehicleWheelSetup& Setup : WheelSetups)
    {
        if (Setup.WheelName == WheelName)
        {
            return &Setup;
        }
    }
    return nullptr;
}

void UWheeledVehicleMovementComponent::EnsureDefaultWheelSetups()
{
    if (!WheelSetups.empty())
    {
        return;
    }

    WheelSetups.push_back(MakeDefaultWheelSetup("FrontLeft",  "wheel_fl", FVector( 1.1f, -0.75f, -0.35f), 35.0f, 0.0f));
    WheelSetups.push_back(MakeDefaultWheelSetup("FrontRight", "wheel_fr", FVector( 1.1f,  0.75f, -0.35f), 35.0f, 0.0f));
    WheelSetups.push_back(MakeDefaultWheelSetup("RearLeft",   "wheel_rl", FVector(-1.1f, -0.75f, -0.35f),  0.0f, 4000.0f));
    WheelSetups.push_back(MakeDefaultWheelSetup("RearRight",  "wheel_rr", FVector(-1.1f,  0.75f, -0.35f),  0.0f, 4000.0f));
}

FVehicleDesc UWheeledVehicleMovementComponent::BuildVehicleDesc() const
{
    FVehicleDesc Desc;

    const AActor*          OwnerActor = GetOwner();
    const USceneComponent* Chassis    = GetUpdatedComponent();

    Desc.Owner.ActorId             = OwnerActor ? OwnerActor->GetUUID() : 0;
    Desc.Owner.ComponentId         = GetUUID();
    Desc.Owner.ComponentGeneration = 1;
    Desc.Owner.Domain              = EPhysicsBodyDomain::Vehicle;

    if (Chassis)
    {
        Desc.WorldTransform = FTransform(Chassis->GetWorldLocation(), Chassis->GetWorldRotation(), FVector(1.0f, 1.0f, 1.0f));
    }

    Desc.ChassisHalfExtents = ChassisHalfExtents;
    Desc.ChassisMass        = ChassisMass;
    Desc.ChassisCMOffset    = ChassisCenterOfMassOffset;

    Desc.EnginePeakTorque = EnginePeakTorque;
    Desc.EngineMaxOmega   = EngineMaxOmega;
    Desc.ClutchStrength   = ClutchStrength;
    Desc.TireFriction     = TireFriction;

    Desc.Wheels.clear();
    Desc.Wheels.reserve(WheelSetups.size());
    for (const FVehicleWheelSetup& Setup : WheelSetups)
    {
        FVehicleWheelDesc Wheel;
        Wheel.WheelName     = Setup.WheelName;
        Wheel.BoneName      = Setup.BoneName;
        Wheel.LocalPosition = Setup.LocalPosition;
        Wheel.Radius        = Setup.Radius;
        Wheel.Width         = Setup.Width;
        Wheel.Mass          = Setup.Mass;
        Wheel.MOI           = 0.5f * Setup.Mass * Setup.Radius * Setup.Radius;

        Wheel.MaxBrakeTorque     = Setup.MaxBrakeTorque;
        Wheel.MaxHandbrakeTorque = Setup.MaxHandbrakeTorque;
        Wheel.MaxSteerRadians    = Setup.MaxSteerAngleDegrees * VehicleDegToRad;

        Wheel.SuspensionMaxCompression   = Setup.SuspensionMaxCompression;
        Wheel.SuspensionMaxDroop         = Setup.SuspensionMaxDroop;
        Wheel.SuspensionSpringStrength   = Setup.SuspensionSpringStrength;
        Wheel.SuspensionSpringDamperRate = Setup.SuspensionSpringDamperRate;
        Wheel.TireType                   = 0;

        Desc.Wheels.push_back(Wheel);
    }

    return Desc;
}

void UWheeledVehicleMovementComponent::RegisterPhysicsSnapshotReceiver()
{
    if (PhysicsSnapshotReceiverHandle != 0 || !VehicleHandle.IsValid())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    TWeakObjectPtr<UWheeledVehicleMovementComponent> WeakThis(this);
    PhysicsSnapshotReceiverHandle = World->RegisterPhysicsSnapshotReceiver(
        [WeakThis](const FPhysicsWorldSnapshot& Snapshot)
        {
            UWheeledVehicleMovementComponent* Component = WeakThis.Get();
            if (!IsValid(Component))
            {
                return;
            }

            Component->ConsumeVehicleSnapshot(Snapshot);
        });
}

void UWheeledVehicleMovementComponent::UnregisterPhysicsSnapshotReceiver()
{
    if (PhysicsSnapshotReceiverHandle == 0)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->UnregisterPhysicsSnapshotReceiver(PhysicsSnapshotReceiverHandle);
    }
    PhysicsSnapshotReceiverHandle = 0;
}

void UWheeledVehicleMovementComponent::ConsumeVehicleSnapshot(const FPhysicsWorldSnapshot& Snapshot)
{
    bHasLastSnapshot = false;

    if (!VehicleHandle.IsValid())
    {
        return;
    }

    const FVehicleSnapshot* Vehicle = Snapshot.FindVehicleByComponent(GetUUID());
    if (!Vehicle || Vehicle->Vehicle != VehicleHandle)
    {
        return;
    }

    ApplyVehicleSnapshot(*Vehicle);
}

void UWheeledVehicleMovementComponent::ApplyVehicleSnapshot(const FVehicleSnapshot& Snapshot)
{
    LastSnapshot = Snapshot;
    bHasLastSnapshot = true;

    if (USceneComponent* Chassis = GetUpdatedComponent())
    {
        Chassis->SetWorldLocation(Snapshot.ChassisWorldTransform.Location);
        Chassis->SetWorldRotation(Snapshot.ChassisWorldTransform.Rotation);
    }
}
