#include "Physics/PhysicsAssetPreviewUtils.h"

#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Mesh/Skeletal/SkeletalMesh.h"
#include "Mesh/Skeletal/SkeletalMeshAsset.h"
#include "Physics/PhysicsAsset.h"

namespace
{
    bool HasMeaningfulBoneName(const FName& BoneName)
    {
        return BoneName.IsValid() && BoneName != FName::None;
    }

    // Preview uses the same authored transform convention as runtime creation, but it
    // stays completely detached from live physics state so editor tools can call it freely.
    FTransform ComposePreviewTransforms(const FTransform& ParentWorld, const FTransform& Local)
    {
        FTransform Result = Local;
        Result.Location = ParentWorld.Location + ParentWorld.Rotation.RotateVector(Local.Location);
        Result.Rotation = (ParentWorld.Rotation * Local.Rotation).GetNormalized();
        Result.Scale = FVector::OneVector;
        return Result;
    }

    FTransform GetComponentWorldTransform(const USkeletalMeshComponent* Component)
    {
        FTransform Result;
        if (!Component)
        {
            return Result;
        }

        Result.Location = Component->GetWorldLocation();
        Result.Rotation = Component->GetWorldMatrix().ToQuat().GetNormalized();
        Result.Scale = FVector::OneVector;
        return Result;
    }

    int32 FindBoneIndexInMesh(const USkeletalMesh* SkeletalMesh, const FName& BoneName)
    {
        if (!SkeletalMesh || !HasMeaningfulBoneName(BoneName))
        {
            return -1;
        }

        const FSkeletalMesh* MeshAsset = SkeletalMesh->GetSkeletalMeshAsset();
        if (!MeshAsset)
        {
            return -1;
        }

        const FString BoneNameString = BoneName.ToString();
        for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(MeshAsset->Bones.size()); ++BoneIndex)
        {
            if (MeshAsset->Bones[BoneIndex].Name == BoneNameString)
            {
                return BoneIndex;
            }
        }

        return -1;
    }
}

bool FPhysicsAssetPreviewUtils::HasPreviewPose(const USkeletalMeshComponent* PreviewComponent)
{
    if (!PreviewComponent || !PreviewComponent->GetSkeletalMesh())
    {
        return false;
    }

    TArray<FTransform> BoneComponentSpaceTransforms;
    PreviewComponent->GetCurrentBoneGlobalTransforms(BoneComponentSpaceTransforms);
    return !BoneComponentSpaceTransforms.empty();
}

bool FPhysicsAssetPreviewUtils::IsBodySetupIndexValid(const UPhysicsAsset* PhysicsAsset, int32 BodyIndex)
{
    return PhysicsAsset &&
        BodyIndex >= 0 &&
        BodyIndex < static_cast<int32>(PhysicsAsset->GetBodySetups().size());
}

bool FPhysicsAssetPreviewUtils::IsConstraintSetupIndexValid(const UPhysicsAsset* PhysicsAsset, int32 ConstraintIndex)
{
    return PhysicsAsset &&
        ConstraintIndex >= 0 &&
        ConstraintIndex < static_cast<int32>(PhysicsAsset->GetConstraintSetups().size());
}

bool FPhysicsAssetPreviewUtils::ResolveBodyBoneIndex(
    const USkeletalMesh* SkeletalMesh,
    const FPhysicsAssetBodySetup& BodySetup,
    int32& OutBoneIndex)
{
    OutBoneIndex = FindBoneIndexInMesh(SkeletalMesh, BodySetup.BoneName);
    return OutBoneIndex >= 0;
}

bool FPhysicsAssetPreviewUtils::ResolveConstraintBoneIndices(
    const USkeletalMesh* SkeletalMesh,
    const FPhysicsAssetConstraintSetup& ConstraintSetup,
    int32& OutParentBoneIndex,
    int32& OutChildBoneIndex)
{
    OutParentBoneIndex = FindBoneIndexInMesh(SkeletalMesh, ConstraintSetup.ParentBoneName);
    OutChildBoneIndex = FindBoneIndexInMesh(SkeletalMesh, ConstraintSetup.ChildBoneName);
    return OutParentBoneIndex >= 0 && OutChildBoneIndex >= 0;
}

bool FPhysicsAssetPreviewUtils::ComputePreviewBodyWorldTransform(
    const USkeletalMeshComponent* PreviewComponent,
    const UPhysicsAsset* PhysicsAsset,
    int32 BodyIndex,
    FTransform& OutWorldTransform)
{
    // Preview helpers fail fast on missing data so editor callers get deterministic
    // "no transform available" behavior instead of partially computed gizmo state.
    if (!PreviewComponent || !PhysicsAsset || !HasPreviewPose(PreviewComponent))
    {
        return false;
    }

    const TArray<FPhysicsAssetBodySetup>& BodySetups = PhysicsAsset->GetBodySetups();
    if (!IsBodySetupIndexValid(PhysicsAsset, BodyIndex))
    {
        return false;
    }

    const USkeletalMesh* SkeletalMesh = PreviewComponent->GetSkeletalMesh();
    int32 BoneIndex = -1;
    if (!ResolveBodyBoneIndex(SkeletalMesh, BodySetups[BodyIndex], BoneIndex))
    {
        return false;
    }

    TArray<FTransform> BoneComponentSpaceTransforms;
    PreviewComponent->GetCurrentBoneGlobalTransforms(BoneComponentSpaceTransforms);
    if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(BoneComponentSpaceTransforms.size()))
    {
        return false;
    }

    const FTransform ComponentWorldTransform = GetComponentWorldTransform(PreviewComponent);
    const FTransform BoneWorldTransform =
        ComposePreviewTransforms(ComponentWorldTransform, BoneComponentSpaceTransforms[BoneIndex]);
    OutWorldTransform = ComposePreviewTransforms(BoneWorldTransform, BodySetups[BodyIndex].BodyLocalFrame);
    return true;
}

bool FPhysicsAssetPreviewUtils::ComputePreviewBodyWorldTransformByBoneName(
    const USkeletalMeshComponent* PreviewComponent,
    const UPhysicsAsset* PhysicsAsset,
    const FName& BoneName,
    FTransform& OutWorldTransform)
{
    if (!PhysicsAsset)
    {
        return false;
    }

    return ComputePreviewBodyWorldTransform(
        PreviewComponent,
        PhysicsAsset,
        PhysicsAsset->FindBodySetupIndexByBoneName(BoneName),
        OutWorldTransform);
}

bool FPhysicsAssetPreviewUtils::ComputePreviewConstraintWorldFrames(
    const USkeletalMeshComponent* PreviewComponent,
    const UPhysicsAsset* PhysicsAsset,
    int32 ConstraintIndex,
    FTransform& OutParentFrameWorld,
    FTransform& OutChildFrameWorld)
{
    if (!PreviewComponent || !PhysicsAsset || !HasPreviewPose(PreviewComponent))
    {
        return false;
    }

    const TArray<FPhysicsAssetConstraintSetup>& ConstraintSetups = PhysicsAsset->GetConstraintSetups();
    if (!IsConstraintSetupIndexValid(PhysicsAsset, ConstraintIndex))
    {
        return false;
    }

    const FPhysicsAssetConstraintSetup& ConstraintSetup = ConstraintSetups[ConstraintIndex];
    FTransform ParentBodyWorld;
    FTransform ChildBodyWorld;
    if (!ComputePreviewBodyWorldTransformByBoneName(
            PreviewComponent,
            PhysicsAsset,
            ConstraintSetup.ParentBoneName,
            ParentBodyWorld) ||
        !ComputePreviewBodyWorldTransformByBoneName(
            PreviewComponent,
            PhysicsAsset,
            ConstraintSetup.ChildBoneName,
            ChildBodyWorld))
    {
        return false;
    }

    // Constraint preview frames are derived from preview body world transforms so editor
    // gizmos match the same local-frame convention used by runtime constraint creation.
    OutParentFrameWorld = ComposePreviewTransforms(ParentBodyWorld, ConstraintSetup.ParentLocalFrame);
    OutChildFrameWorld = ComposePreviewTransforms(ChildBodyWorld, ConstraintSetup.ChildLocalFrame);
    return true;
}
