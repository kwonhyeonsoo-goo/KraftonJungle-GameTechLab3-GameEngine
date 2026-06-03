#pragma once

#include "Cloth/ClothCollisionTypes.h"

class UPhysicsAsset;
class USkeletalMesh;
class USkeletalMeshComponent;
struct FSkeletalClothConfig;
struct FTransform;

class FClothCollisionGatherer
{
public:
    FClothCollisionGatherResult GatherPhysicsAsset(
        const USkeletalMeshComponent& Component,
        const USkeletalMesh& SkeletalMesh,
        const UPhysicsAsset* PhysicsAsset,
        const FSkeletalClothConfig& ClothConfig,
        const FClothCollisionBudget& Budget = FClothCollisionBudget()) const;

private:
    void SelectWithinBudget(
        FClothCollisionGatherResult& Result,
        const FClothCollisionBudget& Budget) const;
};
