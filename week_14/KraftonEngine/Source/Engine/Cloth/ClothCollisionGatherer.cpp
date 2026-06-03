#include "Cloth/ClothCollisionGatherer.h"

#include "Component/Primitive/SkeletalMeshComponent.h"
#include "GameFramework/AActor.h"
#include "Math/Quat.h"
#include "Mesh/Skeletal/SkeletalMesh.h"
#include "Mesh/Skeletal/SkeletalMeshAsset.h"
#include "Physics/PhysicsAsset.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float ClothCollisionScaleTolerance = 1.0e-4f;

    FTransform ComposeClothCollisionTransforms(const FTransform& Parent, const FTransform& Local)
    {
        FTransform Result = Local;
        Result.Location = Parent.Location + Parent.Rotation.RotateVector(Local.Location);
        Result.Rotation = (Parent.Rotation * Local.Rotation).GetNormalized();
        Result.Scale = FVector::OneVector;
        return Result;
    }

    bool IsFiniteFloat(float Value)
    {
        return std::isfinite(Value);
    }

    bool IsFiniteVector(const FVector& Value)
    {
        return IsFiniteFloat(Value.X) && IsFiniteFloat(Value.Y) && IsFiniteFloat(Value.Z);
    }

    bool IsUniformPositiveScale(const FVector& Scale)
    {
        return IsFiniteVector(Scale) &&
            Scale.X > ClothCollisionScaleTolerance &&
            Scale.Y > ClothCollisionScaleTolerance &&
            Scale.Z > ClothCollisionScaleTolerance &&
            std::fabs(Scale.X - Scale.Y) <= ClothCollisionScaleTolerance &&
            std::fabs(Scale.Y - Scale.Z) <= ClothCollisionScaleTolerance;
    }

    bool IsValidTransformForCollision(const FTransform& Transform)
    {
        return IsFiniteVector(Transform.Location) &&
            IsFiniteVector(Transform.Scale) &&
            IsFiniteFloat(Transform.Rotation.X) &&
            IsFiniteFloat(Transform.Rotation.Y) &&
            IsFiniteFloat(Transform.Rotation.Z) &&
            IsFiniteFloat(Transform.Rotation.W);
    }

    int32 FindBoneIndexInMesh(const FSkeletalMesh& MeshAsset, const FName& BoneName)
    {
        if (!BoneName.IsValid() || BoneName == FName::None)
        {
            return -1;
        }

        const FString BoneNameString = BoneName.ToString();
        for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(MeshAsset.Bones.size()); ++BoneIndex)
        {
            if (MeshAsset.Bones[BoneIndex].Name == BoneNameString)
            {
                return BoneIndex;
            }
        }
        return -1;
    }

    void AccumulateGatheredStat(FClothCollisionDebugStats& Stats, EClothCollisionPrimitiveType Type)
    {
        switch (Type)
        {
        case EClothCollisionPrimitiveType::Sphere:
            ++Stats.GatheredSpheres;
            break;
        case EClothCollisionPrimitiveType::Capsule:
            ++Stats.GatheredCapsules;
            break;
        case EClothCollisionPrimitiveType::Box:
            ++Stats.GatheredBoxes;
            break;
        default:
            break;
        }
    }

    void AccumulateSelectedStat(FClothCollisionDebugStats& Stats, EClothCollisionPrimitiveType Type)
    {
        switch (Type)
        {
        case EClothCollisionPrimitiveType::Sphere:
            ++Stats.SelectedSpheres;
            break;
        case EClothCollisionPrimitiveType::Capsule:
            ++Stats.SelectedCapsules;
            break;
        case EClothCollisionPrimitiveType::Box:
            ++Stats.SelectedBoxes;
            break;
        default:
            break;
        }
    }
}

FClothCollisionGatherResult FClothCollisionGatherer::GatherPhysicsAsset(
    const USkeletalMeshComponent& Component,
    const USkeletalMesh& SkeletalMesh,
    const UPhysicsAsset* PhysicsAsset,
    const FSkeletalClothConfig& ClothConfig,
    const FClothCollisionBudget& Budget) const
{
    FClothCollisionGatherResult Result;
    if (!ClothConfig.bEnablePhysicsAssetCollision || !PhysicsAsset)
    {
        return Result;
    }

    FSkeletalMesh* MeshAsset = SkeletalMesh.GetSkeletalMeshAsset();
    if (!MeshAsset)
    {
        return Result;
    }

    TArray<FTransform> BoneComponentSpaceTransforms;
    Component.GetCurrentBoneGlobalTransforms(BoneComponentSpaceTransforms);
    if (BoneComponentSpaceTransforms.empty())
    {
        return Result;
    }

    const uint32 OwnerActorId = Component.GetOwner() ? Component.GetOwner()->GetUUID() : 0;
    const uint32 OwnerComponentId = Component.GetUUID();
    const TArray<FPhysicsAssetBodySetup>& BodySetups = PhysicsAsset->GetBodySetups();
    for (int32 BodyIndex = 0; BodyIndex < static_cast<int32>(BodySetups.size()); ++BodyIndex)
    {
        const FPhysicsAssetBodySetup& BodySetup = BodySetups[BodyIndex];
        const int32 BoneIndex = FindBoneIndexInMesh(*MeshAsset, BodySetup.BoneName);
        if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(BoneComponentSpaceTransforms.size()))
        {
            continue;
        }

        const FTransform BodyComponentTransform =
            ComposeClothCollisionTransforms(BoneComponentSpaceTransforms[BoneIndex], BodySetup.BodyLocalFrame);
        const TArray<FPhysicsAssetShapeSetup>& Shapes = BodySetup.Shapes;
        for (int32 ShapeIndex = 0; ShapeIndex < static_cast<int32>(Shapes.size()); ++ShapeIndex)
        {
            const FPhysicsAssetShapeSetup& ShapeSetup = Shapes[ShapeIndex];
            const bool bIncludeShape = ClothConfig.PhysicsAssetCollisionFilter.empty() ||
                std::find_if(
                    ClothConfig.PhysicsAssetCollisionFilter.begin(),
                    ClothConfig.PhysicsAssetCollisionFilter.end(),
                    [BodyIndex, ShapeIndex](const FSkeletalClothPhysicsAssetCollisionFilterEntry& Entry)
                    {
                        return Entry.BodyIndex == BodyIndex && Entry.ShapeIndex == ShapeIndex;
                    }) != ClothConfig.PhysicsAssetCollisionFilter.end();
            if (!bIncludeShape)
            {
                FClothCollisionCandidate Rejected;
                Rejected.SourceId.Source = EClothCollisionSource::PhysicsAsset;
                Rejected.SourceId.OwnerActorId = OwnerActorId;
                Rejected.SourceId.OwnerComponentId = OwnerComponentId;
                Rejected.SourceId.BoneName = BodySetup.BoneName;
                Rejected.SourceId.BodyIndex = BodyIndex;
                Rejected.SourceId.ShapeIndex = ShapeIndex;
                Rejected.State = EClothCollisionSelectState::RejectedByParticipationFilter;
                Result.Candidates.push_back(Rejected);
                ++Result.Stats.Rejected;
                continue;
            }

            FClothCollisionCandidate Candidate;
            Candidate.SourceId.Source = EClothCollisionSource::PhysicsAsset;
            Candidate.SourceId.OwnerActorId = OwnerActorId;
            Candidate.SourceId.OwnerComponentId = OwnerComponentId;
            Candidate.SourceId.BoneName = BodySetup.BoneName;
            Candidate.SourceId.BodyIndex = BodyIndex;
            Candidate.SourceId.ShapeIndex = ShapeIndex;
            Candidate.StableTieBreaker =
                (static_cast<uint64>(BodyIndex) << 32) | static_cast<uint32>(ShapeIndex);

            switch (ShapeSetup.Type)
            {
            case EPhysicsAssetShapeType::Sphere:
                Candidate.Type = EClothCollisionPrimitiveType::Sphere;
                Candidate.Radius = ShapeSetup.SphereRadius;
                break;
            case EPhysicsAssetShapeType::Capsule:
                Candidate.Type = EClothCollisionPrimitiveType::Capsule;
                Candidate.Radius = ShapeSetup.CapsuleRadius;
                Candidate.CapsuleHalfHeight = ShapeSetup.CapsuleHalfHeight;
                break;
            case EPhysicsAssetShapeType::Box:
                Candidate.Type = EClothCollisionPrimitiveType::Box;
                Candidate.HalfExtent = ShapeSetup.BoxHalfExtent;
                break;
            default:
                Candidate.State = EClothCollisionSelectState::SkippedFilter;
                ++Result.Stats.Rejected;
                Result.Candidates.push_back(Candidate);
                continue;
            }

            Candidate.LocalTransform =
                ComposeClothCollisionTransforms(BodyComponentTransform, ShapeSetup.LocalTransform);

            if (!IsValidTransformForCollision(Candidate.LocalTransform))
            {
                Candidate.State = EClothCollisionSelectState::SkippedInvalidTransform;
                ++Result.Stats.Rejected;
                Result.Candidates.push_back(Candidate);
                continue;
            }

            if (!IsUniformPositiveScale(Candidate.LocalTransform.Scale))
            {
                Candidate.State = EClothCollisionSelectState::SkippedNonUniformScale;
                ++Result.Stats.SkippedNonUniformScale;
                Result.Candidates.push_back(Candidate);
                continue;
            }

            if ((Candidate.Type == EClothCollisionPrimitiveType::Sphere ||
                Candidate.Type == EClothCollisionPrimitiveType::Capsule) &&
                Candidate.Radius <= 0.0f)
            {
                Candidate.State = EClothCollisionSelectState::SkippedInvalidTransform;
                ++Result.Stats.Rejected;
                Result.Candidates.push_back(Candidate);
                continue;
            }

            if (Candidate.Type == EClothCollisionPrimitiveType::Capsule &&
                Candidate.CapsuleHalfHeight <= 0.0f)
            {
                Candidate.State = EClothCollisionSelectState::SkippedInvalidTransform;
                ++Result.Stats.Rejected;
                Result.Candidates.push_back(Candidate);
                continue;
            }

            AccumulateGatheredStat(Result.Stats, Candidate.Type);
            Result.Candidates.push_back(Candidate);
        }
    }

    SelectWithinBudget(Result, Budget);
    return Result;
}

void FClothCollisionGatherer::SelectWithinBudget(
    FClothCollisionGatherResult& Result,
    const FClothCollisionBudget& Budget) const
{
    uint32 SelectedSphereCount = 0;
    uint32 SelectedCapsuleCount = 0;
    uint32 SelectedBoxCount = 0;

    std::sort(
        Result.Candidates.begin(),
        Result.Candidates.end(),
        [](const FClothCollisionCandidate& A, const FClothCollisionCandidate& B)
        {
            return A.StableTieBreaker < B.StableTieBreaker;
        });

    for (FClothCollisionCandidate& Candidate : Result.Candidates)
    {
        if (Candidate.State != EClothCollisionSelectState::Gathered)
        {
            continue;
        }

        bool bSelect = false;
        switch (Candidate.Type)
        {
        case EClothCollisionPrimitiveType::Sphere:
            bSelect = SelectedSphereCount < Budget.MaxAuthoredSpheres;
            if (bSelect)
            {
                ++SelectedSphereCount;
                FClothCollisionSphere Sphere;
                Sphere.LocalCenter = Candidate.LocalTransform.Location;
                Sphere.Radius = Candidate.Radius * Candidate.LocalTransform.Scale.X;
                Result.SelectedPrimitives.Spheres.push_back(Sphere);
            }
            break;
        case EClothCollisionPrimitiveType::Capsule:
            bSelect = SelectedCapsuleCount < Budget.MaxAuthoredCapsules;
            if (bSelect)
            {
                ++SelectedCapsuleCount;
                const FVector LocalAxis = Candidate.LocalTransform.Rotation.RotateVector(FVector(0.0f, 0.0f, 1.0f));
                const float HalfHeight = Candidate.CapsuleHalfHeight * Candidate.LocalTransform.Scale.X;
                FClothCollisionCapsule Capsule;
                Capsule.LocalPoint0 = Candidate.LocalTransform.Location - LocalAxis * HalfHeight;
                Capsule.LocalPoint1 = Candidate.LocalTransform.Location + LocalAxis * HalfHeight;
                Capsule.Radius = Candidate.Radius * Candidate.LocalTransform.Scale.X;
                Result.SelectedPrimitives.Capsules.push_back(Capsule);
            }
            break;
        case EClothCollisionPrimitiveType::Box:
            bSelect = SelectedBoxCount < Budget.MaxAuthoredBoxes &&
                (SelectedBoxCount + 1) * 6 <= Budget.MaxNvClothPlanes &&
                SelectedBoxCount < Budget.MaxNvClothConvexes;
            if (bSelect)
            {
                ++SelectedBoxCount;
                FClothCollisionBox Box;
                Box.LocalTransform = Candidate.LocalTransform;
                Box.HalfExtent = Candidate.HalfExtent * Candidate.LocalTransform.Scale.X;
                Result.SelectedPrimitives.Boxes.push_back(Box);
            }
            break;
        default:
            break;
        }

        if (bSelect)
        {
            Candidate.State = EClothCollisionSelectState::Selected;
            AccumulateSelectedStat(Result.Stats, Candidate.Type);
        }
        else
        {
            Candidate.State = EClothCollisionSelectState::TruncatedByBudget;
            ++Result.Stats.Truncated;
        }
    }
}
