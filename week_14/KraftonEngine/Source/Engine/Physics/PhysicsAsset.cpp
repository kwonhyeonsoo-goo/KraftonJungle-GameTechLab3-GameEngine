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

const FPhysicsAssetBodySetup* UPhysicsAsset::FindBodySetupByBoneName(const FName& BoneName) const
{
    const int32 Index = FindBodySetupIndexByBoneName(BoneName);
    return Index >= 0 ? &BodySetups[Index] : nullptr;
}
