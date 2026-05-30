#pragma once

#include "Core/Types/CoreTypes.h"
#include "Object/Ptr/WeakObjectPtr.h"
#include "Physics/PhysicsBodyHandle.h"

class UPhysicsAsset;
class USkeletalMeshComponent;

// Runtime state is separated from UPhysicsAsset so the asset stays editable/serializable data only.
class FPhysicsAssetInstance
{
public:
    bool Initialize(USkeletalMeshComponent* InOwner, UPhysicsAsset* InAsset);
    bool CreateBodiesAndConstraints();
    void DestroyBodiesAndConstraints();
    void Shutdown();
    void ResetRuntimeState();
    bool HasLivePhysicsObjects() const;

    UPhysicsAsset* GetAsset() const;
    USkeletalMeshComponent* GetOwnerComponent() const;

    const TArray<FPhysicsBodyHandle>& GetBodies() const { return BodiesByBone; }
    const TArray<FPhysicsConstraintHandle>& GetConstraints() const { return Constraints; }
    FPhysicsBodyHandle GetBodyHandleByBoneName(const FName& BoneName) const;
    int32 FindBodySetupIndexByBoneName(const FName& BoneName) const;
    int32 FindBoneIndexForBody(const FName& BoneName) const;
    bool IsInitialized() const { return bInitialized; }
    int32 GetRagdollRootBoneIndex() const { return RagdollRootBoneIndex; }

private:
    TWeakObjectPtr<USkeletalMeshComponent> OwnerComponent;
    TWeakObjectPtr<UPhysicsAsset> SourceAsset;
    TArray<FPhysicsBodyHandle> BodiesByBone;
    TArray<FPhysicsConstraintHandle> Constraints;
    TMap<FString, int32> BoneNameToIndex;
    int32 RagdollRootBoneIndex = -1;
    bool bInitialized = false;
};
