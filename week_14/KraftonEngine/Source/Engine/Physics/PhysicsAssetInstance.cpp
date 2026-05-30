#include "Physics/PhysicsAssetInstance.h"

#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Mesh/Skeletal/SkeletalMesh.h"
#include "Mesh/Skeletal/SkeletalMeshAsset.h"
#include "Physics/PhysicsAsset.h"

bool FPhysicsAssetInstance::Initialize(USkeletalMeshComponent* InOwner, UPhysicsAsset* InAsset)
{
    Shutdown();

    if (!InOwner || !InAsset)
    {
        return false;
    }

    USkeletalMesh* Mesh = InOwner->GetSkeletalMesh();
    FSkeletalMesh* MeshAsset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
    if (!MeshAsset)
    {
        return false;
    }

    OwnerComponent = InOwner;
    SourceAsset = InAsset;

    const TArray<FPhysicsAssetBodySetup>& BodySetups = InAsset->GetBodySetups();
    BodiesByBone.resize(BodySetups.size());
    Constraints.resize(InAsset->GetConstraintSetups().size());

    for (int32 BodyIndex = 0; BodyIndex < static_cast<int32>(BodySetups.size()); ++BodyIndex)
    {
        const FName& BoneName = BodySetups[BodyIndex].BoneName;
        int32 BoneIndex = -1;

        for (int32 MeshBoneIndex = 0; MeshBoneIndex < static_cast<int32>(MeshAsset->Bones.size()); ++MeshBoneIndex)
        {
            if (MeshAsset->Bones[MeshBoneIndex].Name == BoneName.ToString())
            {
                BoneIndex = MeshBoneIndex;
                break;
            }
        }

        BoneNameToIndex[BoneName.ToString()] = BoneIndex;
        if (BodyIndex == 0)
        {
            RagdollRootBoneIndex = BoneIndex;
        }
    }

    bInitialized = true;
    return true;
}

void FPhysicsAssetInstance::Shutdown()
{
    ResetRuntimeState();
    OwnerComponent.Reset();
    SourceAsset.Reset();
    BoneNameToIndex.clear();
    RagdollRootBoneIndex = -1;
    bInitialized = false;
}

void FPhysicsAssetInstance::ResetRuntimeState()
{
    BodiesByBone.clear();
    Constraints.clear();
}

UPhysicsAsset* FPhysicsAssetInstance::GetAsset() const
{
    return SourceAsset.Get();
}

USkeletalMeshComponent* FPhysicsAssetInstance::GetOwnerComponent() const
{
    return OwnerComponent.Get();
}

int32 FPhysicsAssetInstance::FindBoneIndexForBody(const FName& BoneName) const
{
    auto It = BoneNameToIndex.find(BoneName.ToString());
    if (It != BoneNameToIndex.end())
    {
        return It->second;
    }

    return -1;
}
