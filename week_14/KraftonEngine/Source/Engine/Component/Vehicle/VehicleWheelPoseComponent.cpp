#include "Component/Vehicle/VehicleWheelPoseComponent.h"

#include "Component/Movement/WheeledVehicleMovementComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Mesh/Skeletal/SkeletalMesh.h"
#include "Mesh/Skeletal/SkeletalMeshAsset.h"
#include "Component/SceneComponent.h"
#include "GameFramework/AActor.h"
#include "Physics/Vehicle/VehicleTypes.h"


namespace
{
    bool BuildBaseComponentSpaceMatrices(USkeletalMeshComponent* Mesh, TArray<FMatrix>& OutGlobals)
    {
        OutGlobals.clear();
        if (!Mesh)
        {
            return false;
        }

        USkeletalMesh* SkeletalMesh = Mesh->GetSkeletalMesh();
        FSkeletalMesh* Asset = SkeletalMesh ? SkeletalMesh->GetSkeletalMeshAsset() : nullptr;
        if (!Asset || Asset->Bones.empty())
        {
            return false;
        }

        const int32 BoneCount = static_cast<int32>(Asset->Bones.size());
        OutGlobals.resize(BoneCount);

        for (int32 BoneIndex = 0; BoneIndex < BoneCount; ++BoneIndex)
        {
            const FMatrix LocalMatrix = Mesh->GetBoneEditBaseLocalTransformByIndex(BoneIndex).ToMatrix();
            const int32 ParentIndex = Asset->Bones[BoneIndex].ParentIndex;
            OutGlobals[BoneIndex] = ParentIndex >= 0 && ParentIndex < BoneIndex
                ? LocalMatrix * OutGlobals[ParentIndex]
                : LocalMatrix;
        }

        return true;
    }

    FTransform MakeWheelVisualComponentTransform(
        const FTransform& BaseComponentTransform,
        const FVector& ComponentLocation,
        const FVehicleWheelSnapshot& WheelSnapshot
        )
    {
        const FQuat SteerRotation = FQuat::FromAxisAngle(FVector(0.0f, 0.0f, 1.0f), WheelSnapshot.SteerAngle);
        const FQuat SpinRotation  = FQuat::FromAxisAngle(FVector(0.0f, 1.0f, 0.0f), WheelSnapshot.RotationAngle);

        FQuat DeltaRotation = SteerRotation * SpinRotation;
        DeltaRotation.Normalize();

        FQuat TargetRotation = BaseComponentTransform.Rotation * DeltaRotation;
        TargetRotation.Normalize();

        return FTransform(ComponentLocation, TargetRotation, BaseComponentTransform.Scale);
    }

    bool ApplyWheelBonePoseFromSnapshot(
        USkeletalMeshComponent* Mesh,
        const FVehicleWheelSetup& Setup,
        const FVehicleWheelSnapshot& WheelSnapshot
        )
    {
        if (!Mesh || !Setup.BoneName.IsValid())
        {
            return false;
        }

        USkeletalMesh* SkeletalMesh = Mesh->GetSkeletalMesh();
        FSkeletalMesh* Asset = SkeletalMesh ? SkeletalMesh->GetSkeletalMeshAsset() : nullptr;
        if (!Asset)
        {
            return false;
        }

        const int32 BoneIndex = Mesh->FindBoneIndex(Setup.BoneName);
        if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(Asset->Bones.size()))
        {
            return false;
        }

        TArray<FMatrix> BaseGlobals;
        if (!BuildBaseComponentSpaceMatrices(Mesh, BaseGlobals) || BoneIndex >= static_cast<int32>(BaseGlobals.size()))
        {
            return false;
        }

        const FMatrix ComponentWorldInverse = Mesh->GetWorldMatrix().GetInverse();
        const FVector ComponentLocation = ComponentWorldInverse.TransformPositionWithW(WheelSnapshot.WorldTransform.Location);

        const FTransform BaseComponentTransform(BaseGlobals[BoneIndex]);
        const FTransform TargetComponentTransform = MakeWheelVisualComponentTransform(BaseComponentTransform, ComponentLocation, WheelSnapshot);

        const int32 ParentIndex = Asset->Bones[BoneIndex].ParentIndex;
        const FMatrix TargetComponentMatrix = TargetComponentTransform.ToMatrix();
        const FMatrix LocalMatrix = ParentIndex >= 0 && ParentIndex < static_cast<int32>(BaseGlobals.size())
            ? TargetComponentMatrix * BaseGlobals[ParentIndex].GetInverse()
            : TargetComponentMatrix;

        Mesh->SetBoneLocalTransformByIndex(BoneIndex, FTransform(LocalMatrix));
        return true;
    }
}

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
            if (ApplyWheelBonePoseFromSnapshot(Mesh.Get(), *Setup, WheelSnapshot))
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
