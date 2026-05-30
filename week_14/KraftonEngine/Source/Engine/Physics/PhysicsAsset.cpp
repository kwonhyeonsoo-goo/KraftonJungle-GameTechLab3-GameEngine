#include "Physics/PhysicsAsset.h"

#include <algorithm>

namespace
{
    bool HasMeaningfulBoneName(const FName& BoneName)
    {
        return BoneName.IsValid() && BoneName != FName::None;
    }

    bool IsSameConstraintPair(
        const FPhysicsAssetConstraintSetup& Setup,
        const FName& ParentBoneName,
        const FName& ChildBoneName)
    {
        return Setup.ParentBoneName == ParentBoneName && Setup.ChildBoneName == ChildBoneName;
    }
}

void UPhysicsAsset::Serialize(FArchive& Ar)
{
    UObject::Serialize(Ar);

    Ar << SkeletonBinding;
    Ar << BodySetups;
    Ar << ConstraintSetups;
}

int32 UPhysicsAsset::AddBodySetup(const FPhysicsAssetBodySetup& InBodySetup)
{
    // Placeholder bodies with no assigned bone are allowed during authoring.
    if (HasMeaningfulBoneName(InBodySetup.BoneName) && HasBodySetupForBone(InBodySetup.BoneName))
    {
        return -1;
    }

    BodySetups.push_back(InBodySetup);
    return static_cast<int32>(BodySetups.size()) - 1;
}

bool UPhysicsAsset::RemoveBodySetupByIndex(int32 BodyIndex)
{
    if (BodyIndex < 0 || BodyIndex >= static_cast<int32>(BodySetups.size()))
    {
        return false;
    }

    const FName RemovedBoneName = BodySetups[BodyIndex].BoneName;
    BodySetups.erase(BodySetups.begin() + BodyIndex);

    if (HasMeaningfulBoneName(RemovedBoneName))
    {
        // Removing a body also removes dangling constraints that named the same bone so
        // tool code does not have to clean them up in a second pass.
        ConstraintSetups.erase(
            std::remove_if(
                ConstraintSetups.begin(),
                ConstraintSetups.end(),
                [&](const FPhysicsAssetConstraintSetup& ConstraintSetup)
                {
                    return ConstraintSetup.ParentBoneName == RemovedBoneName ||
                           ConstraintSetup.ChildBoneName == RemovedBoneName;
                }),
            ConstraintSetups.end());
    }

    return true;
}

bool UPhysicsAsset::RemoveBodySetupByBoneName(const FName& BoneName)
{
    return RemoveBodySetupByIndex(FindBodySetupIndexByBoneName(BoneName));
}

bool UPhysicsAsset::UpdateBodySetup(int32 BodyIndex, const FPhysicsAssetBodySetup& InBodySetup)
{
    if (BodyIndex < 0 || BodyIndex >= static_cast<int32>(BodySetups.size()))
    {
        return false;
    }

    if (HasMeaningfulBoneName(InBodySetup.BoneName))
    {
        const int32 ExistingBodyIndex = FindBodySetupIndexByBoneName(InBodySetup.BoneName);
        if (ExistingBodyIndex >= 0 && ExistingBodyIndex != BodyIndex)
        {
            return false;
        }
    }

    BodySetups[BodyIndex] = InBodySetup;
    return true;
}

void UPhysicsAsset::ClearBodySetups()
{
    BodySetups.clear();
    ConstraintSetups.clear();
}

int32 UPhysicsAsset::AddConstraintSetup(const FPhysicsAssetConstraintSetup& InConstraintSetup)
{
    // Placeholder constraints are allowed until both ends are assigned to real bones.
    if (HasMeaningfulBoneName(InConstraintSetup.ParentBoneName) &&
        HasMeaningfulBoneName(InConstraintSetup.ChildBoneName) &&
        FindConstraintSetupIndex(InConstraintSetup.ParentBoneName, InConstraintSetup.ChildBoneName) >= 0)
    {
        return -1;
    }

    ConstraintSetups.push_back(InConstraintSetup);
    return static_cast<int32>(ConstraintSetups.size()) - 1;
}

bool UPhysicsAsset::RemoveConstraintSetupByIndex(int32 ConstraintIndex)
{
    if (ConstraintIndex < 0 || ConstraintIndex >= static_cast<int32>(ConstraintSetups.size()))
    {
        return false;
    }

    ConstraintSetups.erase(ConstraintSetups.begin() + ConstraintIndex);
    return true;
}

bool UPhysicsAsset::RemoveConstraintSetup(const FName& ParentBoneName, const FName& ChildBoneName)
{
    return RemoveConstraintSetupByIndex(FindConstraintSetupIndex(ParentBoneName, ChildBoneName));
}

bool UPhysicsAsset::UpdateConstraintSetup(int32 ConstraintIndex, const FPhysicsAssetConstraintSetup& InConstraintSetup)
{
    if (ConstraintIndex < 0 || ConstraintIndex >= static_cast<int32>(ConstraintSetups.size()))
    {
        return false;
    }

    if (HasMeaningfulBoneName(InConstraintSetup.ParentBoneName) &&
        HasMeaningfulBoneName(InConstraintSetup.ChildBoneName))
    {
        const int32 ExistingConstraintIndex =
            FindConstraintSetupIndex(InConstraintSetup.ParentBoneName, InConstraintSetup.ChildBoneName);
        if (ExistingConstraintIndex >= 0 && ExistingConstraintIndex != ConstraintIndex)
        {
            return false;
        }
    }

    ConstraintSetups[ConstraintIndex] = InConstraintSetup;
    return true;
}

void UPhysicsAsset::ClearConstraintSetups()
{
    ConstraintSetups.clear();
}

int32 UPhysicsAsset::FindBodySetupIndexByBoneName(const FName& BoneName) const
{
    if (!HasMeaningfulBoneName(BoneName))
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

FPhysicsAssetBodySetup* UPhysicsAsset::FindMutableBodySetupByBoneName(const FName& BoneName)
{
    const int32 Index = FindBodySetupIndexByBoneName(BoneName);
    return Index >= 0 ? &BodySetups[Index] : nullptr;
}

UPhysicsAsset::EEditorSetupState UPhysicsAsset::GetBodySetupEditorState(int32 BodyIndex) const
{
    if (BodyIndex < 0 || BodyIndex >= static_cast<int32>(BodySetups.size()))
    {
        return EEditorSetupState::Invalid;
    }

    const FPhysicsAssetBodySetup& BodySetup = BodySetups[BodyIndex];
    if (!HasMeaningfulBoneName(BodySetup.BoneName))
    {
        return EEditorSetupState::Placeholder;
    }

    if (BodySetup.Shapes.empty())
    {
        return EEditorSetupState::Invalid;
    }

    const int32 ExistingIndex = FindBodySetupIndexByBoneName(BodySetup.BoneName);
    if (ExistingIndex >= 0 && ExistingIndex != BodyIndex)
    {
        return EEditorSetupState::Invalid;
    }

    return EEditorSetupState::RuntimeReady;
}

int32 UPhysicsAsset::FindConstraintSetupIndex(const FName& ParentBoneName, const FName& ChildBoneName) const
{
    if (!HasMeaningfulBoneName(ParentBoneName) || !HasMeaningfulBoneName(ChildBoneName))
    {
        return -1;
    }

    for (int32 Index = 0; Index < static_cast<int32>(ConstraintSetups.size()); ++Index)
    {
        if (IsSameConstraintPair(ConstraintSetups[Index], ParentBoneName, ChildBoneName))
        {
            return Index;
        }
    }

    return -1;
}

bool UPhysicsAsset::HasConstraintBetweenBones(const FName& ParentBoneName, const FName& ChildBoneName) const
{
    return FindConstraintSetupIndex(ParentBoneName, ChildBoneName) >= 0;
}

const FPhysicsAssetConstraintSetup* UPhysicsAsset::FindConstraintSetup(
    const FName& ParentBoneName,
    const FName& ChildBoneName) const
{
    const int32 Index = FindConstraintSetupIndex(ParentBoneName, ChildBoneName);
    return Index >= 0 ? &ConstraintSetups[Index] : nullptr;
}

FPhysicsAssetConstraintSetup* UPhysicsAsset::FindMutableConstraintSetup(
    const FName& ParentBoneName,
    const FName& ChildBoneName)
{
    const int32 Index = FindConstraintSetupIndex(ParentBoneName, ChildBoneName);
    return Index >= 0 ? &ConstraintSetups[Index] : nullptr;
}

TArray<const FPhysicsAssetConstraintSetup*> UPhysicsAsset::FindConstraintSetupsForBone(const FName& BoneName) const
{
    TArray<const FPhysicsAssetConstraintSetup*> Result;

    if (!HasMeaningfulBoneName(BoneName))
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

UPhysicsAsset::EEditorSetupState UPhysicsAsset::GetConstraintSetupEditorState(int32 ConstraintIndex) const
{
    if (ConstraintIndex < 0 || ConstraintIndex >= static_cast<int32>(ConstraintSetups.size()))
    {
        return EEditorSetupState::Invalid;
    }

    const FPhysicsAssetConstraintSetup& ConstraintSetup = ConstraintSetups[ConstraintIndex];
    if (!HasMeaningfulBoneName(ConstraintSetup.ParentBoneName) ||
        !HasMeaningfulBoneName(ConstraintSetup.ChildBoneName))
    {
        return EEditorSetupState::Placeholder;
    }

    if (ConstraintSetup.ParentBoneName == ConstraintSetup.ChildBoneName)
    {
        return EEditorSetupState::Invalid;
    }

    const int32 ExistingIndex =
        FindConstraintSetupIndex(ConstraintSetup.ParentBoneName, ConstraintSetup.ChildBoneName);
    if (ExistingIndex >= 0 && ExistingIndex != ConstraintIndex)
    {
        return EEditorSetupState::Invalid;
    }

    return EEditorSetupState::RuntimeReady;
}
