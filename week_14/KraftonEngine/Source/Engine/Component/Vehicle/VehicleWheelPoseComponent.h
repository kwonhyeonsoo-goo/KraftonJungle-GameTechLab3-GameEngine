#pragma once

#include "Component/ActorComponent.h"
#include "Object/FName.h"
#include "Object/Ptr/WeakObjectPtr.h"

#include "Source/Engine/Component/Vehicle/VehicleWheelPoseComponent.generated.h"

class UWheeledVehicleMovementComponent;
class USceneComponent;
class USkeletalMeshComponent;

/**
 * Applies vehicle wheel visual poses after physics snapshots have been consumed.
 *
 * This component intentionally owns the visual side of wheels. Vehicle movement no longer
 * guesses child components or writes wheel transforms directly. First-pass support drives
 * skeletal bones via FVehicleWheelSetup::BoneName. If no matching bone exists, it can
 * fall back to an explicitly named SceneComponent via VisualComponentName.
 */
UCLASS()
class UVehicleWheelPoseComponent : public UActorComponent
{
public:
    GENERATED_BODY()

    UVehicleWheelPoseComponent();
    ~UVehicleWheelPoseComponent() override = default;

    void BeginPlay() override;
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;

    UFUNCTION(Callable, Category="Vehicle|Visual")
    void SetVehicleMovement(UWheeledVehicleMovementComponent* InMovement);
    UFUNCTION(Pure, Category="Vehicle|Visual")
    UWheeledVehicleMovementComponent* GetVehicleMovement() const { return VehicleMovement.Get(); }

    UFUNCTION(Callable, Category="Vehicle|Visual")
    void SetSkeletalMeshComponent(USkeletalMeshComponent* InMesh);
    UFUNCTION(Pure, Category="Vehicle|Visual")
    USkeletalMeshComponent* GetSkeletalMeshComponent() const { return Mesh.Get(); }

private:
    USceneComponent* FindVisualSceneComponentByName(const FName& ComponentName) const;

private:
    TWeakObjectPtr<UWheeledVehicleMovementComponent> VehicleMovement;
    TWeakObjectPtr<USkeletalMeshComponent> Mesh;
};
