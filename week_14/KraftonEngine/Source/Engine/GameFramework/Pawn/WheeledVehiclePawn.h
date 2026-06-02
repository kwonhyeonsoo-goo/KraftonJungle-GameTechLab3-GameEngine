#pragma once

#include "GameFramework/Pawn/Pawn.h"
#include "Object/Ptr/WeakObjectPtr.h"

#include "Source/Engine/GameFramework/Pawn/WheeledVehiclePawn.generated.h"

class USkeletalMeshComponent;
class UWheeledVehicleMovementComponent;
class UVehicleWheelPoseComponent;
class USpringArmComponent;
class UCameraComponent;

// Possess 가능한 표준 차량 Pawn.
//
// 차량은 더 이상 임의 Actor + VehicleMovementComponent 조합에 의존하지 않는다. 기본 구성은
// SkeletalMesh(root) + WheeledVehicleMovement + WheelPose + SpringArm/Camera 이며, 입력은
// APawn::InputComponent를 통해 VehicleMovement로 들어간다.
UCLASS()
class AWheeledVehiclePawn : public APawn
{
public:
    GENERATED_BODY()
    AWheeledVehiclePawn() = default;
    ~AWheeledVehiclePawn() override = default;

    virtual void InitDefaultComponents(const FString& SkeletalMeshFileName);
    void BeginPlay() override;
    void PostDuplicate() override;

protected:
    void OnPostLoad(FArchive& Ar) override;

public:

    UFUNCTION(Pure, Category="Vehicle|Components")
    USkeletalMeshComponent* GetMesh() const { return Mesh; }
    UFUNCTION(Pure, Category="Vehicle|Components")
    UWheeledVehicleMovementComponent* GetVehicleMovement() const { return VehicleMovement; }
    UFUNCTION(Pure, Category="Vehicle|Components")
    UVehicleWheelPoseComponent* GetWheelPoseComponent() const { return WheelPose; }
    UFUNCTION(Pure, Category="Vehicle|Components")
    USpringArmComponent* GetSpringArm() const { return SpringArm; }
    UFUNCTION(Pure, Category="Vehicle|Components")
    UCameraComponent* GetCamera() const { return Camera; }

protected:
    void SetupInputComponent() override;
    void RebindVehicleComponents();

protected:
    TWeakObjectPtr<USkeletalMeshComponent> Mesh = nullptr;
    TWeakObjectPtr<UWheeledVehicleMovementComponent> VehicleMovement = nullptr;
    TWeakObjectPtr<UVehicleWheelPoseComponent> WheelPose = nullptr;
    TWeakObjectPtr<USpringArmComponent> SpringArm = nullptr;
    TWeakObjectPtr<UCameraComponent> Camera = nullptr;

    UPROPERTY(Edit, Save, Category="Vehicle|Input", DisplayName="Auto Vehicle Input")
    bool bAutoInputVehicle = true;
};
