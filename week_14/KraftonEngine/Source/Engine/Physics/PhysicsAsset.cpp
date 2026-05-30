#include "Physics/PhysicsAsset.h"

void UPhysicsAsset::Serialize(FArchive& Ar)
{
    UObject::Serialize(Ar);

    Ar << SkeletonBinding;
    Ar << BodySetups;
    Ar << ConstraintSetups;
}

int32 UPhysicsAsset::FindBodySetupIndexByBoneName(const FName& BoneName) const
{
    if (!BoneName.IsValid() || BoneName == FName::None)
    {
        return -1;
    }

    for (int32 Index = 0; Index < static_cast<int32>(BodySetups.size()); ++Index)
    {
        if (BodySetups[Index].BoneName == BoneName)
        {
            return Index;
        }
    }

    return -1;
}

bool UPhysicsAsset::HasBodySetupForBone(const FName& BoneName) const
{
    return FindBodySetupIndexByBoneName(BoneName) >= 0;
}

const FPhysicsAssetBodySetup* UPhysicsAsset::FindBodySetupByBoneName(const FName& BoneName) const
{
    const int32 Index = FindBodySetupIndexByBoneName(BoneName);
    return Index >= 0 ? &BodySetups[Index] : nullptr;
}

bool UPhysicsAsset::HasConstraintBetweenBones(const FName& ParentBoneName, const FName& ChildBoneName) const
{
    return FindConstraintSetup(ParentBoneName, ChildBoneName) != nullptr;
}

const FPhysicsAssetConstraintSetup* UPhysicsAsset::FindConstraintSetup(const FName& ParentBoneName, const FName& ChildBoneName) const
{
    if (!ParentBoneName.IsValid() || ParentBoneName == FName::None ||
        !ChildBoneName.IsValid() || ChildBoneName == FName::None)
    {
        return nullptr;
    }

    for (const FPhysicsAssetConstraintSetup& ConstraintSetup : ConstraintSetups)
    {
        if (ConstraintSetup.ParentBoneName == ParentBoneName && ConstraintSetup.ChildBoneName == ChildBoneName)
        {
            return &ConstraintSetup;
        }
    }

    return nullptr;
}

TArray<const FPhysicsAssetConstraintSetup*> UPhysicsAsset::FindConstraintSetupsForBone(const FName& BoneName) const
{
    TArray<const FPhysicsAssetConstraintSetup*> Result;

    if (!BoneName.IsValid() || BoneName == FName::None)
    {
        return Result;
    }

    for (const FPhysicsAssetConstraintSetup& ConstraintSetup : ConstraintSetups)
    {
        if (ConstraintSetup.ParentBoneName == BoneName || ConstraintSetup.ChildBoneName == BoneName)
        {
            Result.push_back(&ConstraintSetup);
        }
    }

    return Result;
}
