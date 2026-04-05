#include "BVHMesh.h"
#include <algorithm>


void FBVHMesh::Build(const FBoundingBox& ObjectBoxes, TArray<Triangle> triangles)
{
    Nodes.clear();

    Triangles = triangles;
    
	SplitRecursive(ObjectBoxes, 0, Triangles.size(), 0);
}

int32 FBVHMesh::SplitRecursive(const FBoundingBox& CurrentBounds, int32 Start, int32 End, int32 Depth)
{
    // 1. 현재 노드 생성 및 등록
    FBVHMeshNode NewNode = {};
    NewNode.Bounds = CurrentBounds;
    NewNode.startIndex = Start;
    NewNode.endIndex = End;

    // 일단 배열에 추가하고 인덱스를 확보
    int32 CurrentNodeIndex = Nodes.size();
    Nodes.push_back(NewNode);
    // 2. 종료 조건 (최대 깊이 도달 혹은 삼각형 개수가 너무 적음)
    int32 NumTris = End - Start;
    if (Depth >= maxDepth || NumTris <= 2)
    {
        return CurrentNodeIndex; // 리프 노드로서 반환
    }

    // 3. 분할 축 결정 (가장 긴 축)
    FVector Size = CurrentBounds.Max - CurrentBounds.Min;
    int32 SplitAxis = 0;
    if (Size.Y > Size.X && Size.Y > Size.Z) SplitAxis = 1;
    else if (Size.Z > Size.X && Size.Z > Size.Y) SplitAxis = 2;

    float SplitPivot = CurrentBounds.GetCenter()[SplitAxis];

    // 4. 삼각형 재정렬 (Partitioning)
    int32 Mid = Start;
    for (int32 i = Start; i < End; ++i)
    {
        FVector TriCenter = (Triangles[i].Vertex1 + Triangles[i].Vertex2 + Triangles[i].Vertex3) / 3.0f;

        // 선택된 축의 중심값을 기준으로 이진 분류
        if (TriCenter[SplitAxis] < SplitPivot)
        {
            std::swap(Triangles[i], Triangles[Mid]);
            Mid++;
        }
    }

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
