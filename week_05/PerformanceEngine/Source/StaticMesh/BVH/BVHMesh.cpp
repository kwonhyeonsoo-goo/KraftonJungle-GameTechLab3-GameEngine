#include "BVHMesh.h"
#include <algorithm>

namespace {
    float SurfaceArea(const FBoundingBox& Box)
    {
        FVector E = Box.Max - Box.Min;
        return 2.0f * (E.X * E.Y + E.Y * E.Z + E.Z * E.X);
    }
}
void FBVHMesh::Build(const FBoundingBox& ObjectBoxes, TArray<Triangle> triangles)
{
    Nodes.clear();

    Triangles = triangles;
    
	SplitRecursive(ObjectBoxes, 0, Triangles.size(), 0);
}

int32 FBVHMesh::SplitRecursive(const FBoundingBox& CurrentBounds, int32 Start, int32 End, int32 Depth)
{
    // 1. 현재 노드 생성 및 등록
    int32 CurrentNodeIndex = Nodes.size();
    FBVHMeshNode NewNode = {};
    NewNode.Bounds = CurrentBounds;
    NewNode.startIndex = Start;
    NewNode.endIndex = End;
    NewNode.LeftChild = -1;
    NewNode.RightChild = -1;
    Nodes.push_back(NewNode);
    // 2. 종료 조건 (최대 깊이 도달 혹은 삼각형 개수가 너무 적음)
    int32 NumTris = End - Start;
    if (Depth >= maxDepth || NumTris <= 4)  // 2→4로 변경
        return CurrentNodeIndex;

    // 3. 분할 축 결정 (가장 긴 축)
    FVector Size = CurrentBounds.Max - CurrentBounds.Min;
    int32 SplitAxis = 0;
    if (Size.Y > Size.X && Size.Y > Size.Z) SplitAxis = 1;
    else if (Size.Z > Size.X && Size.Z > Size.Y) SplitAxis = 2;

    // 축 길이가 0이면 강제 분할
    if (Size[SplitAxis] < 1e-8f)
    {
        int32 Mid = Start + NumTris / 2;
        int32 LeftIdx = SplitRecursive(calcBounds(Triangles, Start, Mid), Start, Mid, Depth + 1);
        int32 RightIdx = SplitRecursive(calcBounds(Triangles, Mid, End), Mid, End, Depth + 1);
        Nodes[CurrentNodeIndex].LeftChild = LeftIdx;
        Nodes[CurrentNodeIndex].RightChild = RightIdx;
        return CurrentNodeIndex;
    }

    // SAH 버킷 구성
    struct Bucket { int32 Count = 0; FBoundingBox Bounds; };
    constexpr int32 NumBuckets = 12;
    Bucket Buckets[NumBuckets];

    float AxisMin = CurrentBounds.Min[SplitAxis];
    float AxisLen = Size[SplitAxis];

    for (int32 i = Start; i < End; ++i)
    {
        FVector TriCenter = (Triangles[i].Vertex1 + Triangles[i].Vertex2 + Triangles[i].Vertex3) / 3.0f;

        int32 b = static_cast<int32>(NumBuckets * ((TriCenter[SplitAxis] - AxisMin) / AxisLen));
        b = std::clamp(b, 0, NumBuckets - 1);
        Buckets[b].Count++;
        Buckets[b].Bounds.Encapsulate(Triangles[i].Vertex1);
        Buckets[b].Bounds.Encapsulate(Triangles[i].Vertex2);
        Buckets[b].Bounds.Encapsulate(Triangles[i].Vertex3);
        // SAH 비용으로 최적 분할점 탐색
        float ParentArea = SurfaceArea(CurrentBounds);
        float BestCost = std::numeric_limits<float>::max();
        int32 BestSplit = NumBuckets / 2;  // fallback

        for (int32 split = 1; split < NumBuckets; ++split)
        {
            // 왼쪽 누적
            FBoundingBox LeftBounds;
            int32 LeftCount = 0;
            for (int32 i = 0; i < split; ++i)
            {
                if (Buckets[i].Count == 0) continue;
                LeftBounds.Encapsulate(Buckets[i].Bounds.Min);
                LeftBounds.Encapsulate(Buckets[i].Bounds.Max);
                LeftCount += Buckets[i].Count;
            }

            // 오른쪽 누적
            FBoundingBox RightBounds;
            int32 RightCount = 0;
            for (int32 i = split; i < NumBuckets; ++i) {
                if (Buckets[i].Count == 0) continue;
                RightBounds.Encapsulate(Buckets[i].Bounds.Min);
                RightBounds.Encapsulate(Buckets[i].Bounds.Max);
                RightCount += Buckets[i].Count;
            }

            if (LeftCount == 0 || RightCount == 0) continue;

            float Cost = (LeftCount * SurfaceArea(LeftBounds) + RightCount * SurfaceArea(RightBounds)) / ParentArea;
            if (Cost < BestCost)
            {
                BestCost = Cost;
                BestSplit = split;
            }
        }

        // BestSplit 기준으로 삼각형 재정렬 (std::partition)
        float SplitPos = AxisMin + AxisLen * (static_cast<float>(BestSplit) / NumBuckets);
        auto MidPtr = std::partition(Triangles.begin() + Start, Triangles.begin() + End,
            [SplitAxis, SplitPos](const Triangle& Tri)
            {
                float Center = (Tri.Vertex1[SplitAxis] + Tri.Vertex2[SplitAxis] + Tri.Vertex3[SplitAxis]) / 3.0f;
                return Center < SplitPos;
            });

        int32 Mid = static_cast<int32>(MidPtr - Triangles.begin());

        // 한쪽으로 몰린 경우 강제 2등분
        // 예외 처리: 한쪽으로 몰리는 경우 강제 분할
        if (Mid == Start || Mid == End)
        {
            Mid = Start + NumTris / 2;
        }

        // 5. 자식 노드의 정확한 Bounds 계산
        FBoundingBox LeftBounds = calcBounds(Triangles, Start, Mid);
        FBoundingBox RightBounds = calcBounds(Triangles, Mid, End);

        // 6. 재귀 호출 및 인덱스 연결
        // 주의: Nodes.Add가 발생하면 메모리 위치가 변하므로, 
        // 반드시 반환된 인덱스를 나중에 대입해야 합니다.
        int32 LeftIdx = SplitRecursive(LeftBounds, Start, Mid, Depth + 1);
        int32 RightIdx = SplitRecursive(RightBounds, Mid, End, Depth + 1);

        // 인덱스를 통해 노드 데이터 갱신
        Nodes[CurrentNodeIndex].LeftChild = LeftIdx;
        Nodes[CurrentNodeIndex].RightChild = RightIdx;

        return CurrentNodeIndex;
    }
}

FBoundingBox FBVHMesh::calcBounds(const TArray<Triangle>& triangles, int start, int end)
{
    FBoundingBox box;

    // 유효하지 않은 범위 체크
    if (start >= end) return box;

    for (int i = start; i < end; ++i) {
        // 삼각형의 세 정점을 모두 박스에 포함시킴
        box.Encapsulate(triangles[i].Vertex1);
        box.Encapsulate(triangles[i].Vertex2);
        box.Encapsulate(triangles[i].Vertex3);
    }

    return box;
}
