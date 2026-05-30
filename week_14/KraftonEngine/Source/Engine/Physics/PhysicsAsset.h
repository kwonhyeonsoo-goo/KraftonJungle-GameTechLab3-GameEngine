#pragma once

#include "Object/Object.h"
#include "Animation/Skeleton/SkeletonTypes.h"
#include "Physics/PhysicsAssetTypes.h"

#include "Source/Engine/Physics/PhysicsAsset.generated.h"

UCLASS()
class UPhysicsAsset : public UObject
{
public:
    GENERATED_BODY()
    UPhysicsAsset()           = default;
    ~UPhysicsAsset() override = default;

    void Serialize(FArchive& Ar) override;
    bool ShouldReflectProperties() const override { return false; }

    const FString& GetAssetPathFileName() const
    {
        return AssetPathFileName;
    }

    void SetAssetPathFileName(const FString& InPath)
    {
        AssetPathFileName = InPath;
    }

    void SetSkeletonBinding(const FSkeletonBinding& InBinding)
    {
        SkeletonBinding = InBinding;
    }

    const FSkeletonBinding& GetSkeletonBinding() const
    {
        return SkeletonBinding;
    }

    const TArray<FPhysicsAssetBodySetup>& GetBodySetups() const
    {
        return BodySetups;
    }

    TArray<FPhysicsAssetBodySetup>& GetMutableBodySetups()
    {
        return BodySetups;
    }

    const TArray<FPhysicsAssetConstraintSetup>& GetConstraintSetups() const
    {
        return ConstraintSetups;
    }

    TArray<FPhysicsAssetConstraintSetup>& GetMutableConstraintSetups()
    {
        return ConstraintSetups;
    }

    // Asset-side lookup helpers keep future skeletal/ragdoll code from
    // re-implementing the same bone/constraint queries at each call site.
    int32 FindBodySetupIndexByBoneName(const FName& BoneName) const;
    bool HasBodySetupForBone(const FName& BoneName) const;
    const FPhysicsAssetBodySetup* FindBodySetupByBoneName(const FName& BoneName) const;
    bool HasConstraintBetweenBones(const FName& ParentBoneName, const FName& ChildBoneName) const;
    const FPhysicsAssetConstraintSetup* FindConstraintSetup(const FName& ParentBoneName, const FName& ChildBoneName) const;
    TArray<const FPhysicsAssetConstraintSetup*> FindConstraintSetupsForBone(const FName& BoneName) const;

private:
    FString AssetPathFileName = "None";

    FSkeletonBinding SkeletonBinding;
    TArray<FPhysicsAssetBodySetup> BodySetups;
    TArray<FPhysicsAssetConstraintSetup> ConstraintSetups;
};
