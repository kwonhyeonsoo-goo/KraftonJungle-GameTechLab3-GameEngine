#pragma once

#include "Math/Transform.h"
#include "Physics/PhysicsAssetTypes.h"

class UPhysicsAsset;
class USkeletalMesh;
class USkeletalMeshComponent;

// Editor preview math stays separate from runtime ragdoll execution so tools can
// place gizmos without requiring live physics objects.
class FPhysicsAssetPreviewUtils
{
public:
    static bool ResolveBodyBoneIndex(const USkeletalMesh* SkeletalMesh, const FPhysicsAssetBodySetup& BodySetup, int32& OutBoneIndex);
    static bool ResolveConstraintBoneIndices(
        const USkeletalMesh* SkeletalMesh,
        const FPhysicsAssetConstraintSetup& ConstraintSetup,
        int32& OutParentBoneIndex,
        int32& OutChildBoneIndex);

    static bool ComputePreviewBodyWorldTransform(
        const USkeletalMeshComponent* PreviewComponent,
        const UPhysicsAsset* PhysicsAsset,
        int32 BodyIndex,
        FTransform& OutWorldTransform);

    static bool ComputePreviewBodyWorldTransformByBoneName(
        const USkeletalMeshComponent* PreviewComponent,
        const UPhysicsAsset* PhysicsAsset,
        const FName& BoneName,
        FTransform& OutWorldTransform);

    static bool ComputePreviewConstraintWorldFrames(
        const USkeletalMeshComponent* PreviewComponent,
        const UPhysicsAsset* PhysicsAsset,
        int32 ConstraintIndex,
        FTransform& OutParentFrameWorld,
        FTransform& OutChildFrameWorld);
};
