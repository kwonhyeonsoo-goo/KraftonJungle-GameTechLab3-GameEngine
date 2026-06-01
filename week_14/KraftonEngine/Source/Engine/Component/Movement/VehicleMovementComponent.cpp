#include "Component/Movement/VehicleMovementComponent.h"

#include "Component/SceneComponent.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"
#include "Physics/IPhysicsScene.h"

#include <algorithm>
#include <cmath>
#include <Object/GarbageCollection.h>

namespace
{
	constexpr float VehicleDegToRad = 3.14159265358979323846f / 180.0f;

	IPhysicsScene* GetPhysicsScene(const UActorComponent* Component)
	{
		UWorld* World = Component ? Component->GetWorld() : nullptr;
		return World ? World->GetPhysicsScene() : nullptr;
	}
}

void UVehicleMovementComponent::BeginPlay()
{
	// UpdatedComponent(= chassis) 를 해석한다. 기본은 액터 Root.
	UMovementComponent::BeginPlay();

	// 씬에 배치된 휠 메시(섀시의 자식)를 위치 기준으로 자동 바인딩.
	ResolveWheelComponents();

	if (VehicleHandle.IsValid())
	{
		return;
	}

	IPhysicsScene* PhysicsScene = GetPhysicsScene(this);
	if (!PhysicsScene || !GetUpdatedComponent())
	{
		return;
	}

	VehicleHandle = PhysicsScene->CreateVehicle(BuildVehicleDesc());
}

void UVehicleMovementComponent::EndPlay()
{
	if (VehicleHandle.IsValid())
	{
		if (IPhysicsScene* PhysicsScene = GetPhysicsScene(this))
		{
			PhysicsScene->DestroyVehicle(VehicleHandle);
		}
		VehicleHandle = FPhysXVehicleHandle {};
	}

	UMovementComponent::EndPlay();
}

void UVehicleMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
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

void UVehicleMovementComponent::AddReferencedObjects(FReferenceCollector& Collector)
{
	UMovementComponent::AddReferencedObjects(Collector);

	Collector.AddReferencedObject(WheelFrontLeft, "UVehicleMovementComponent.WheelFrontLeft");
	Collector.AddReferencedObject(WheelFrontRight, "UVehicleMovementComponent.WheelFrontRight");
	Collector.AddReferencedObject(WheelRearLeft, "UVehicleMovementComponent.WheelRearLeft");
	Collector.AddReferencedObject(WheelRearRight, "UVehicleMovementComponent.WheelRearRight");
}

void UVehicleMovementComponent::SetThrottleInput(float InThrottle)
{
	CurrentInput.Throttle = std::clamp(InThrottle, 0.0f, 1.0f);
}

void UVehicleMovementComponent::SetBrakeInput(float InBrake)
{
	CurrentInput.Brake = std::clamp(InBrake, 0.0f, 1.0f);
}

void UVehicleMovementComponent::SetSteerInput(float InSteer)
{
	CurrentInput.Steer = std::clamp(InSteer, -1.0f, 1.0f);
}

void UVehicleMovementComponent::SetHandbrakeInput(float InHandbrake)
{
	CurrentInput.Handbrake = std::clamp(InHandbrake, 0.0f, 1.0f);
}

void UVehicleMovementComponent::ResetVehicle()
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

void UVehicleMovementComponent::ApplyVehicleSnapshot(const FPhysXVehicleSnapshot& Snapshot)
{
	// chassis 를 먼저 갱신한다. 휠이 chassis 의 자식이면 여기서 함께 따라오지만,
	// 아래에서 휠 world transform 을 절대값으로 덮어쓰므로 결과는 동일하다.
	if (USceneComponent* Chassis = GetUpdatedComponent())
	{
		Chassis->SetWorldLocation(Snapshot.ChassisWorldTransform.Location);
		Chassis->SetWorldRotation(Snapshot.ChassisWorldTransform.Rotation);
	}

	USceneComponent* WheelComponents[4] = {
		WheelFrontLeft.GetRaw(),
		WheelFrontRight.GetRaw(),
		WheelRearLeft.GetRaw(),
		WheelRearRight.GetRaw(),
	};

	for (int32 WheelIndex = 0; WheelIndex < 4; ++WheelIndex)
	{
		USceneComponent* Wheel = WheelComponents[WheelIndex];
		if (!IsValid(Wheel))
		{
			continue;
		}

		const FPhysXVehicleWheelSnapshot& WheelSnapshot = Snapshot.Wheels[WheelIndex];
		Wheel->SetWorldLocation(WheelSnapshot.WorldTransform.Location);
		Wheel->SetWorldRotation(WheelSnapshot.WorldTransform.Rotation);
	}
}

FPhysXVehicleDesc UVehicleMovementComponent::BuildVehicleDesc() const
{
	FPhysXVehicleDesc Desc;

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

	const float SteerRadians = MaxSteerAngleDegrees * VehicleDegToRad;
	const float FrontX       = FrontAxleOffsetX;
	const float RearX        = -RearAxleOffsetX;
	const float HalfTrack    = WheelHalfTrackWidth;
	const float WheelZ       = WheelCenterHeight;

	auto SetupWheel = [&](EPhysXVehicleWheelIndex Index, const FVector& LocalPosition, bool bSteerable, bool bHandbrake)
	{
		FPhysXVehicleWheelDesc& Wheel = Desc.Wheels[static_cast<int32>(Index)];

		Wheel.LocalPosition      = LocalPosition;
		Wheel.Radius             = WheelRadius;
		Wheel.Width              = WheelWidth;
		Wheel.Mass               = WheelMass;
		Wheel.MOI                = 0.5f * WheelMass * WheelRadius * WheelRadius;
		Wheel.MaxBrakeTorque     = MaxBrakeTorque;
		Wheel.MaxHandbrakeTorque = bHandbrake ? MaxHandbrakeTorque : 0.0f;
		Wheel.MaxSteerRadians    = bSteerable ? SteerRadians : 0.0f;

		Wheel.SuspensionMaxCompression   = SuspensionMaxCompression;
		Wheel.SuspensionMaxDroop         = SuspensionMaxDroop;
		Wheel.SuspensionSpringStrength   = SuspensionSpringStrength;
		Wheel.SuspensionSpringDamperRate = SuspensionSpringDamperRate;
		Wheel.TireType                   = 0;
	};

	SetupWheel(EPhysXVehicleWheelIndex::FrontLeft, FVector(FrontX, -HalfTrack, WheelZ), true, false);
	SetupWheel(EPhysXVehicleWheelIndex::FrontRight, FVector(FrontX, HalfTrack, WheelZ), true, false);
	SetupWheel(EPhysXVehicleWheelIndex::RearLeft, FVector(RearX, -HalfTrack, WheelZ), false, true);
	SetupWheel(EPhysXVehicleWheelIndex::RearRight, FVector(RearX, HalfTrack, WheelZ), false, true);

	return Desc;
}

void UVehicleMovementComponent::ResolveWheelComponents()
{
	USceneComponent* Chassis = GetUpdatedComponent();
	if (!Chassis)
	{
		return;
	}

	const float FrontX    = FrontAxleOffsetX;
	const float RearX     = -RearAxleOffsetX;
	const float HalfTrack = WheelHalfTrackWidth;
	const float WheelZ    = WheelCenterHeight;

	const FVector ExpectedLocalPositions[4] = {
		FVector(FrontX, -HalfTrack, WheelZ), // FrontLeft
		FVector(FrontX, HalfTrack, WheelZ),  // FrontRight
		FVector(RearX, -HalfTrack, WheelZ),  // RearLeft
		FVector(RearX, HalfTrack, WheelZ),   // RearRight
	};

	TObjectPtr<USceneComponent>* WheelSlots[4] = {
		&WheelFrontLeft,
		&WheelFrontRight,
		&WheelRearLeft,
		&WheelRearRight,
	};

	// 기대 위치에서 이 반경 안에 있는 자식만 휠로 인정 (차체/다른 자식 오바인딩 방지).
	const float ToleranceSq = 0.5f * 0.5f;

	for (int32 WheelIndex = 0; WheelIndex < 4; ++WheelIndex)
	{
		if (IsValid(WheelSlots[WheelIndex]->GetRaw()))
		{
			continue;
		}

		USceneComponent* BestChild  = nullptr;
		float            BestDistSq = ToleranceSq;

		for (USceneComponent* Child : Chassis->GetChildren())
		{
			if (!IsValid(Child))
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(Child->GetRelativeLocation(), ExpectedLocalPositions[WheelIndex]);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestChild  = Child;
			}
		}

		if (BestChild)
		{
			*WheelSlots[WheelIndex] = BestChild;
		}
	}
}
