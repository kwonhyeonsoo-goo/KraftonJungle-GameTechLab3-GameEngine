#pragma once

#include "Core/Types/CoreTypes.h"

class UPhysicsAsset;
class USkeletalMesh;

struct FPhysicsAssetValidationIssue
{
    enum class ESeverity : uint8
    {
        Warning,
        Error
    };

    FString Message;
    int32 BodyIndex = -1;
    int32 ConstraintIndex = -1;
    ESeverity Severity = ESeverity::Error;
};

class FPhysicsAssetValidation
{
public:
    static bool ValidateBodySetup(
        const USkeletalMesh* SkeletalMesh,
        const UPhysicsAsset* PhysicsAsset,
        int32 BodyIndex,
        FPhysicsAssetValidationIssue* OutIssue = nullptr);

    static bool ValidateConstraintSetup(
        const USkeletalMesh* SkeletalMesh,
        const UPhysicsAsset* PhysicsAsset,
        int32 ConstraintIndex,
        FPhysicsAssetValidationIssue* OutIssue = nullptr);

    static bool ValidateAll(
        const USkeletalMesh* SkeletalMesh,
        const UPhysicsAsset* PhysicsAsset,
        TArray<FPhysicsAssetValidationIssue>& OutIssues);
};
