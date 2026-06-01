#include "Component/Vehicle/VehicleWheelPoseComponent.h"

#include "Component/Movement/WheeledVehicleMovementComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Component/SceneComponent.h"
#include "GameFramework/AActor.h"
#include "Physics/Vehicle/VehicleTypes.h"

UVehicleWheelPoseComponent::UVehicleWheelPoseComponent()
{
    PrimaryComponentTick.SetTickGroup(TG_PostUpdateWork);
    PrimaryComponentTick.SetEndTickGroup(TG_PostUpdateWork);
}

void UVehicleWheelPoseComponent::BeginPlay()
{
    UActorComponent::BeginPlay();

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    if (!VehicleMovement)
    {
        VehicleMovement = Owner->GetComponentByClass<UWheeledVehicleMovementComponent>();
    }

    if (!Mesh)
    {
        Mesh = Owner->GetComponentByClass<USkeletalMeshComponent>();
    }
}

void UVehicleWheelPoseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
{
    UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UWheeledVehicleMovementComponent* Movement = VehicleMovement.Get();
    if (!IsValid(Movement))
    {
        return;
    }

    const FVehicleSnapshot* Snapshot = Movement->GetLastVehicleSnapshot();
    if (!Snapshot)
    {
        return;
    }

    for (const FVehicleWheelSnapshot& WheelSnapshot : Snapshot->Wheels)
    {
        const FVehicleWheelSetup* Setup = Movement->FindWheelSetup(WheelSnapshot.WheelName);
        if (!Setup)
        {
            continue;
        }

        if (Setup->BoneName.IsValid() && IsValid(Mesh.Get()))
        {
            if (Mesh->SetBoneWorldTransformByName(Setup->BoneName, WheelSnapshot.WorldTransform))
            {
                continue;
            }
        }

        if (Setup->VisualComponentName.IsValid())
        {
            if (USceneComponent* VisualComponent = FindVisualSceneComponentByName(Setup->VisualComponentName))
            {
                VisualComponent->SetWorldLocation(WheelSnapshot.WorldTransform.Location);
                VisualComponent->SetWorldRotation(WheelSnapshot.WorldTransform.Rotation);
            }
        }
    }
}

void UVehicleWheelPoseComponent::SetVehicleMovement(UWheeledVehicleMovementComponent* InMovement)
{
    VehicleMovement = InMovement;
}

void UVehicleWheelPoseComponent::SetSkeletalMeshComponent(USkeletalMeshComponent* InMesh)
{
    Mesh = InMesh;
}

USceneComponent* UVehicleWheelPoseComponent::FindVisualSceneComponentByName(const FName& ComponentName) const
{
    if (!ComponentName.IsValid())
    {
        return nullptr;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return nullptr;
    }

    const FString WantedName = ComponentName.ToString();
    for (UActorComponent* Component : Owner->GetComponents())
    {
        USceneComponent* SceneComponent = Cast<USceneComponent>(Component);
        if (!IsValid(SceneComponent))
        {
            continue;
        }

        if (SceneComponent->GetName() == WantedName)
        {
            return SceneComponent;
        }
    }

    return nullptr;
}
