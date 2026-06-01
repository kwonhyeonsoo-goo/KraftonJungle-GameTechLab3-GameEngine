#pragma once

#include "Component/Movement/MovementComponent.h"
#include "Object/Ptr/ObjectPtr.h"
#include "Physics/PhysXVehicleTypes.h"

#include "Source/Engine/Component/Movement/VehicleMovementComponent.generated.h"

/**
 * PhysX Drive4W raycast 차량을 구동하는 이동 컴포넌트.
 *
 * UpdatedComponent(보통 액터 Root)를 chassis 로 사용한다. BeginPlay 에서 현재 프로퍼티로
 * FPhysXVehicleDesc 를 만들어 PhysicsScene 에 차량을 생성하고, 매 Tick 에서 입력(throttle/
 * brake/steer/handbrake)을 물리 스레드로 전달한다. 물리 스냅샷이 publish 되면 World 가
 * ApplyVehicleSnapshot() 을 호출해 chassis + 휠 SceneComponent 의 visual transform 을 갱신한다.
 *
 * 입력은 PlayerController/Lua 등 외부에서 Set*Input() 으로 주입한다 (기본값 0 = 정지).
 */
UCLASS()
class UVehicleMovementComponent : public UMovementComponent
{
public:
	GENERATED_BODY()

	UVehicleMovementComponent()           = default;
	~UVehicleMovementComponent() override = default;

	void BeginPlay() override;
	void EndPlay() override;
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;
	void AddReferencedObjects(FReferenceCollector& Collector) override;

	// --- 입력 주입 (Throttle/Brake/Handbrake 는 0~1, Steer 는 -1~1 로 clamp) ---
	UFUNCTION(Callable, Category="Vehicle|Input")
	void SetThrottleInput(float InThrottle);
	UFUNCTION(Callable, Category="Vehicle|Input")
	void SetBrakeInput(float InBrake);
	UFUNCTION(Callable, Category="Vehicle|Input")
	void SetSteerInput(float InSteer);
	UFUNCTION(Callable, Category="Vehicle|Input")
	void SetHandbrakeInput(float InHandbrake);

	// 차량을 현재 chassis 위치/자세로 리셋 (속도 0, 기어 1단).
	UFUNCTION(Callable, Category="Vehicle")
	void ResetVehicle();

	// Physics snapshot receiver 경로에서 호출 — chassis + 휠 visual transform 갱신.
	void ApplyVehicleSnapshot(const FPhysXVehicleSnapshot& Snapshot);

	bool                IsVehicleCreated() const { return VehicleHandle.IsValid(); }
	FPhysXVehicleHandle GetVehicleHandle() const { return VehicleHandle; }

private:
	FPhysXVehicleDesc BuildVehicleDesc() const;
	void RegisterPhysicsSnapshotReceiver();
	void UnregisterPhysicsSnapshotReceiver();

	// 휠 visual 슬롯이 비어 있으면, 섀시의 자식 SceneComponent 중 각 휠의 기대 local 위치에
	// 가장 가까운 것을 자동으로 바인딩한다. (휠 ref 는 Transient 라 씬에 저장되지 않으므로
	// 씬에 배치된 차량도 이 자동 바인딩으로 바퀴를 연결한다 — 이름이 아니라 위치로 매칭.)
	void ResolveWheelComponents();

	// --- 섀시 ---
	UPROPERTY(Edit, Save, Category="Vehicle|Chassis", DisplayName="Chassis Half Extents", Type=Vec3, Speed=0.05f)
	FVector ChassisHalfExtents = FVector(1.25f, 0.6f, 0.35f);
	UPROPERTY(Edit, Save, Category="Vehicle|Chassis", DisplayName="Chassis Mass", Speed=1.0f)
	float ChassisMass = 1200.0f;
	UPROPERTY(Edit, Save, Category="Vehicle|Chassis", DisplayName="Center Of Mass Offset", Type=Vec3, Speed=0.01f)
	FVector ChassisCenterOfMassOffset = FVector(0.0f, 0.0f, -0.25f);

	// --- 엔진/구동 ---
	UPROPERTY(Edit, Save, Category="Vehicle|Engine", DisplayName="Engine Peak Torque", Speed=1.0f)
	float EnginePeakTorque = 500.0f;
	UPROPERTY(Edit, Save, Category="Vehicle|Engine", DisplayName="Engine Max Omega", Speed=1.0f)
	float EngineMaxOmega = 600.0f;
	UPROPERTY(Edit, Save, Category="Vehicle|Engine", DisplayName="Clutch Strength", Speed=0.1f)
	float ClutchStrength = 10.0f;

	// --- 타이어 ---
	UPROPERTY(Edit, Save, Category="Vehicle|Tire", DisplayName="Tire Friction", Speed=0.05f)
	float TireFriction = 1.5f;

	// --- 휠 공통 ---
	UPROPERTY(Edit, Save, Category="Vehicle|Wheel", DisplayName="Wheel Radius", Speed=0.01f)
	float WheelRadius = 0.35f;
	UPROPERTY(Edit, Save, Category="Vehicle|Wheel", DisplayName="Wheel Width", Speed=0.01f)
	float WheelWidth = 0.25f;
	UPROPERTY(Edit, Save, Category="Vehicle|Wheel", DisplayName="Wheel Mass", Speed=0.1f)
	float WheelMass = 20.0f;
	UPROPERTY(Edit, Save, Category="Vehicle|Wheel", DisplayName="Max Steer Angle (deg)", Speed=0.5f)
	float MaxSteerAngleDegrees = 35.0f;
	UPROPERTY(Edit, Save, Category="Vehicle|Wheel", DisplayName="Max Brake Torque", Speed=10.0f)
	float MaxBrakeTorque = 1500.0f;
	UPROPERTY(Edit, Save, Category="Vehicle|Wheel", DisplayName="Max Handbrake Torque", Speed=10.0f)
	float MaxHandbrakeTorque = 4000.0f;

	// --- 휠 배치 (chassis local 기준) ---
	UPROPERTY(Edit, Save, Category="Vehicle|Wheel", DisplayName="Front Axle Offset X", Speed=0.01f)
	float FrontAxleOffsetX = 1.1f;
	UPROPERTY(Edit, Save, Category="Vehicle|Wheel", DisplayName="Rear Axle Offset X", Speed=0.01f)
	float RearAxleOffsetX = 1.1f;
	UPROPERTY(Edit, Save, Category="Vehicle|Wheel", DisplayName="Wheel Half Track Width", Speed=0.01f)
	float WheelHalfTrackWidth = 0.75f;
	UPROPERTY(Edit, Save, Category="Vehicle|Wheel", DisplayName="Wheel Center Height", Speed=0.01f)
	float WheelCenterHeight = -0.35f;

	// --- 서스펜션 ---
	UPROPERTY(Edit, Save, Category="Vehicle|Suspension", DisplayName="Spring Strength", Speed=100.0f)
	float SuspensionSpringStrength = 35000.0f;
	UPROPERTY(Edit, Save, Category="Vehicle|Suspension", DisplayName="Spring Damper Rate", Speed=10.0f)
	float SuspensionSpringDamperRate = 4500.0f;
	UPROPERTY(Edit, Save, Category="Vehicle|Suspension", DisplayName="Suspension Max Compression", Speed=0.01f)
	float SuspensionMaxCompression = 0.30f;
	UPROPERTY(Edit, Save, Category="Vehicle|Suspension", DisplayName="Suspension Max Droop", Speed=0.01f)
	float SuspensionMaxDroop = 0.10f;

	// --- 휠 visual 대상 (할당되지 않으면 무시) ---
	// Scene/component topology 는 명시적으로 재구성되므로 Save 직렬화하지 않는다.
	UPROPERTY(Transient, Category="Vehicle|Visual", DisplayName="Wheel Front Left")
	TObjectPtr<USceneComponent> WheelFrontLeft = nullptr;
	UPROPERTY(Transient, Category="Vehicle|Visual", DisplayName="Wheel Front Right")
	TObjectPtr<USceneComponent> WheelFrontRight = nullptr;
	UPROPERTY(Transient, Category="Vehicle|Visual", DisplayName="Wheel Rear Left")
	TObjectPtr<USceneComponent> WheelRearLeft = nullptr;
	UPROPERTY(Transient, Category="Vehicle|Visual", DisplayName="Wheel Rear Right")
	TObjectPtr<USceneComponent> WheelRearRight = nullptr;

	// --- 런타임 상태 (직렬화 안 함) ---
	FPhysXVehicleHandle     VehicleHandle;
	FPhysXVehicleInputState CurrentInput;
	uint64                  PhysicsSnapshotReceiverHandle = 0;
};
