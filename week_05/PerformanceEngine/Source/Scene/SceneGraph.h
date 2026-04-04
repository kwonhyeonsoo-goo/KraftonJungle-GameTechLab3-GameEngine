#pragma once
#include "Types/Array.h"
#include "Types/PlatformTypes.h"
#include "Math/Vector.h"
#include "Math/BoundingBox.h"
#include "Scene/SceneTypes.h"
#include "Types/Set.h"

struct FVisibilityResults;
class FScene;

class FSceneNode {
public:
    int32 PrimitiveIndex = -1;
    FVector Center;
    FBoundingBox Volume;

    int32 Parent = -1;
    TArray<int32> Children;  // 포인터 대신 인덱스
};

class FSceneGraph
{
public:
    void Build(const FScene& InScene);
    void Pick(const FRay& InRay, const FVisibilityResults& CandidateVisibilityResults, TArray<int32>& OutCandidates) const;


private:
    void Build(const TArray<FBoundingBox>& ObjectBoxes);

    int32 BuildRecursive(TArray<int32>& Indices, const FBoundingBox& NodeVolume, int32 Depth);

    void PickRecursive(int32 NodeIndex, const FRay& InRay, const TArray<int32>& Candidate, TArray<int32>& OutCandidates, int32& MAX) const;

    TArray<FSceneNode> Nodes;
    int32 RootIndex = -1;
};

