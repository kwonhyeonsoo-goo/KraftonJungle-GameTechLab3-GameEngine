#include "Physics/PhysicsAssetValidation.h"

#include "Animation/Skeleton/SkeletonManager.h"
#include "Mesh/Skeletal/SkeletalMesh.h"
#include "Physics/PhysicsAsset.h"
#include "Physics/PhysicsAssetPreviewUtils.h"

namespace
{
    bool HasMeaningfulBoneName(const FName& BoneName)
    {
        return BoneName.IsValid() && BoneName != FName::None;
    }

    void AppendIssue(
        TArray<FPhysicsAssetValidationIssue>& OutIssues,
        const FString& Message,
        int32 BodyIndex = -1,
        int32 ConstraintIndex = -1)
    {
        FPhysicsAssetValidationIssue Issue;
        Issue.Message = Message;
        Issue.BodyIndex = BodyIndex;
        Issue.ConstraintIndex = ConstraintIndex;
        OutIssues.push_back(Issue);
    }
}

bool FPhysicsAssetValidation::ValidateBodySetup(
    const USkeletalMesh* SkeletalMesh,
    const UPhysicsAsset* PhysicsAsset,
    int32 BodyIndex,
    FPhysicsAssetValidationIssue* OutIssue)
{
    TArray<FPhysicsAssetValidationIssue> Issues;
    ValidateAll(SkeletalMesh, PhysicsAsset, Issues);

    for (const FPhysicsAssetValidationIssue& Issue : Issues)
    {
        if (Issue.BodyIndex == BodyIndex)
        {
            if (OutIssue)
            {
                *OutIssue = Issue;
            }
            return false;
        }
    }

    if (!Issues.empty())
    {
        if (OutIssue)
        {
            *OutIssue = Issues.front();
        }
        return false;
    }

    return true;
}

bool FPhysicsAssetValidation::ValidateConstraintSetup(
    const USkeletalMesh* SkeletalMesh,
    const UPhysicsAsset* PhysicsAsset,
    int32 ConstraintIndex,
    FPhysicsAssetValidationIssue* OutIssue)
{
    TArray<FPhysicsAssetValidationIssue> Issues;
    ValidateAll(SkeletalMesh, PhysicsAsset, Issues);

    for (const FPhysicsAssetValidationIssue& Issue : Issues)
    {
        if (Issue.ConstraintIndex == ConstraintIndex)
        {
            if (OutIssue)
            {
                *OutIssue = Issue;
            }
            return false;
        }
    }

    if (!Issues.empty())
    {
        if (OutIssue)
        {
            *OutIssue = Issues.front();
        }
        return false;
    }

    return true;
}

bool FPhysicsAssetValidation::ValidateAll(
    const USkeletalMesh* SkeletalMesh,
    const UPhysicsAsset* PhysicsAsset,
    TArray<FPhysicsAssetValidationIssue>& OutIssues)
{
    OutIssues.clear();

    if (!SkeletalMesh)
    {
        AppendIssue(OutIssues, "target skeletal mesh is null");
        return false;
    }

    if (!PhysicsAsset)
    {
        AppendIssue(OutIssues, "physics asset is null");
        return false;
    }

    const FSkeletonCompatibilityReport CompatibilityReport =
        FSkeletonManager::CheckCompatibility(SkeletalMesh->GetSkeletonBinding(), PhysicsAsset->GetSkeletonBinding());
    if (CompatibilityReport.Result != ESkeletonCompatibilityResult::ExactSkeleton)
    {
        AppendIssue(
            OutIssues,
            FString("physics asset skeleton mismatch: ") + CompatibilityReport.Reason);
    }

    const TArray<FPhysicsAssetBodySetup>& BodySetups = PhysicsAsset->GetBodySetups();
    const TArray<FPhysicsAssetConstraintSetup>& ConstraintSetups = PhysicsAsset->GetConstraintSetups();

    for (int32 BodyIndex = 0; BodyIndex < static_cast<int32>(BodySetups.size()); ++BodyIndex)
    {
        const FPhysicsAssetBodySetup& BodySetup = BodySetups[BodyIndex];
        if (!HasMeaningfulBoneName(BodySetup.BoneName))
        {
            AppendIssue(OutIssues, "body setup has no target bone", BodyIndex);
        }
        else
        {
            int32 BoneIndex = -1;
            if (!FPhysicsAssetPreviewUtils::ResolveBodyBoneIndex(SkeletalMesh, BodySetup, BoneIndex))
            {
                AppendIssue(
                    OutIssues,
                    FString("body setup references a missing bone: ") + BodySetup.BoneName.ToString(),
                    BodyIndex);
            }

            const int32 ExistingBodyIndex = PhysicsAsset->FindBodySetupIndexByBoneName(BodySetup.BoneName);
            if (ExistingBodyIndex >= 0 && ExistingBodyIndex != BodyIndex)
            {
                AppendIssue(
                    OutIssues,
                    FString("duplicate body setup for bone: ") + BodySetup.BoneName.ToString(),
                    BodyIndex);
            }
        }

        if (BodySetup.Shapes.empty())
        {
            AppendIssue(OutIssues, "body setup has no shapes", BodyIndex);
        }
    }

    for (int32 ConstraintIndex = 0; ConstraintIndex < static_cast<int32>(ConstraintSetups.size()); ++ConstraintIndex)
    {
        const FPhysicsAssetConstraintSetup& ConstraintSetup = ConstraintSetups[ConstraintIndex];

        if (!HasMeaningfulBoneName(ConstraintSetup.ParentBoneName))
        {
            AppendIssue(OutIssues, "constraint setup has no parent bone", -1, ConstraintIndex);
        }

        if (!HasMeaningfulBoneName(ConstraintSetup.ChildBoneName))
        {
            AppendIssue(OutIssues, "constraint setup has no child bone", -1, ConstraintIndex);
        }

        if (HasMeaningfulBoneName(ConstraintSetup.ParentBoneName) &&
            ConstraintSetup.ParentBoneName == ConstraintSetup.ChildBoneName)
        {
            AppendIssue(OutIssues, "constraint setup uses the same parent and child bone", -1, ConstraintIndex);
        }

        int32 ParentBoneIndex = -1;
        int32 ChildBoneIndex = -1;
        if (!FPhysicsAssetPreviewUtils::ResolveConstraintBoneIndices(
                SkeletalMesh,
                ConstraintSetup,
                ParentBoneIndex,
                ChildBoneIndex))
        {
            if (HasMeaningfulBoneName(ConstraintSetup.ParentBoneName) && ParentBoneIndex < 0)
            {
                AppendIssue(
                    OutIssues,
                    FString("constraint parent bone is missing: ") + ConstraintSetup.ParentBoneName.ToString(),
                    -1,
                    ConstraintIndex);
            }

            if (HasMeaningfulBoneName(ConstraintSetup.ChildBoneName) && ChildBoneIndex < 0)
            {
                AppendIssue(
                    OutIssues,
                    FString("constraint child bone is missing: ") + ConstraintSetup.ChildBoneName.ToString(),
                    -1,
                    ConstraintIndex);
            }
        }

        const int32 ExistingConstraintIndex =
            PhysicsAsset->FindConstraintSetupIndex(ConstraintSetup.ParentBoneName, ConstraintSetup.ChildBoneName);
        if (ExistingConstraintIndex >= 0 && ExistingConstraintIndex != ConstraintIndex)
        {
            AppendIssue(
                OutIssues,
                FString("duplicate constraint setup for pair: ") +
                    ConstraintSetup.ParentBoneName.ToString() + " -> " +
                    ConstraintSetup.ChildBoneName.ToString(),
                -1,
                ConstraintIndex);
        }

        if (HasMeaningfulBoneName(ConstraintSetup.ParentBoneName) &&
            !PhysicsAsset->HasBodySetupForBone(ConstraintSetup.ParentBoneName))
        {
            AppendIssue(
                OutIssues,
                FString("constraint parent body is missing: ") + ConstraintSetup.ParentBoneName.ToString(),
                -1,
                ConstraintIndex);
        }

        if (HasMeaningfulBoneName(ConstraintSetup.ChildBoneName) &&
            !PhysicsAsset->HasBodySetupForBone(ConstraintSetup.ChildBoneName))
        {
            AppendIssue(
                OutIssues,
                FString("constraint child body is missing: ") + ConstraintSetup.ChildBoneName.ToString(),
                -1,
                ConstraintIndex);
        }
    }

    return OutIssues.empty();
}
