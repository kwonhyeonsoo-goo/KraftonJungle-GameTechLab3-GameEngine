#pragma once
#include "Types/Array.h"
#include "Types/PlatformTypes.h"
#include "Math/Vector.h"
#include "Math/BoundingBox.h"
#include "Scene/SceneTypes.h"
#include "Types/Set.h"

struct FVisibilityResults;
struct FScenePrimitiveRuntimeData;
class FScene;

// 32바이트 정렬을 맞춰주면 AVX 레지스터 로드 속도가 가장 빠릅니다.
struct alignas(32) FSceneNode
{
    int32 PrimitiveStartIndex = -1;
    int32 PrimitiveCount = 0;//단일 인덱스 대신, 글로벌 배열의 범위를 가리킴
    FBoundingBox Volume;
    FVector Center;
    int32 Parent = -1;

    // 대신 고정 배열 사용 
    int32 ChildIndices[8];
    int32 ChildCount = 0;

    // 8-Way SIMD 교차 검사를 위해 자식들의 AABB를 SoA 형태로 캐싱
    float ChildMinX[8];
    float ChildMinY[8];
    float ChildMinZ[8];
    float ChildMaxX[8];
    float ChildMaxY[8];
    float ChildMaxZ[8];

    FSceneNode()
    {
        // 초기화 시 모든 자식 인덱스는 -1로, 
        // 바운딩 박스는 절대 충돌할 수 없는 무한대 값으로 세팅합니다.
        for (int i = 0; i < 8; ++i)
        {
            ChildIndices[i] = -1;
            ChildMinX[i] = ChildMinY[i] = ChildMinZ[i] = std::numeric_limits<float>::infinity();
            ChildMaxX[i] = ChildMaxY[i] = ChildMaxZ[i] = -std::numeric_limits<float>::infinity();
        }
    }
};

class FSceneGraph
{
public:
    void Reset();

    void Build(const FScene& InScene);
    void Pick(const FRay& InRay, const FVisibilityResults& CandidateVisibilityResults, const TArray<FScenePrimitiveRuntimeData>& PrimitiveBoxes, TArray<int32>& OutCandidates) const;

	const TArray<FSceneNode>& GetNodes() const { return Nodes; }
	const TArray<int32>& GetPrimitiveIndexBuffer() const { return PrimitiveIndexBuffer; }
    int32 GetRootIndex() const { return RootIndex; }

private:
    void Build(const TArray<FBoundingBox>& ObjectBoxes);

    int32 BuildRecursive(const TArray<int32>& Indices, const TArray<FBoundingBox>& ObjectBoxes, const FBoundingBox& NodeVolume, int32 Depth);    
  
    TArray<FSceneNode> Nodes;
    int32 RootIndex = -1;

    TArray<int32> PrimitiveIndexBuffer;
};

