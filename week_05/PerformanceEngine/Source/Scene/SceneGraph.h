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
    FBoundingBox Volume;
    FVector Center = FVector::ZeroVector;
    int32 Parent = -1;
    TArray<int32> Children;
    uint32 PrimitiveIndex = -1;
    // 단일 인덱스 대신 배열로 변경하여 데이터 유실 원천 차단
    TArray<int32> PrimitiveIndices;
};

class FSceneGraph
{
public:
    void Build(const FScene& InScene);
    void Pick(const FRay& InRay, const FVisibilityResults& CandidateVisibilityResults, TArray<int32>& OutCandidates) const;


private:
    void Build(const TArray<FBoundingBox>& ObjectBoxes);

    int32 BuildRecursive(const TArray<int32>& Indices, const FBoundingBox& NodeVolume, int32 Depth);

    void PickRecursive(int32 NodeIndex, const FRay& InRay, const TArray<int32>& Candidate, TArray<int32>& OutCandidates, int32& InOutMaxT) const;

    TArray<FSceneNode> Nodes;
    int32 RootIndex = -1;
};

