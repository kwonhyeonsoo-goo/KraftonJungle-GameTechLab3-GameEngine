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

    Nodes8Way.clear();
    if (!Nodes.empty())
    {
        CollapseTo8Way(0);
		
		TriangleBlocks.clear();
		for (auto& Node8 : Nodes8Way)
		{
			for (int32 i = 0; i < Node8.ValidChildCount; ++i)
			{
				if (Node8.ChildIndices[i] == -1) // Leaf node
				{
					int32 OriginalStart = Node8.TriangleStart[i];
					int32 OriginalCount = Node8.TriangleCount[i];

					int32 BlockStart = TriangleBlocks.size();
					int32 NumBlocks = (OriginalCount + 7) / 8;

					for (int32 b = 0; b < NumBlocks; ++b)
					{
						FTriangleBlock8 Block;
						Block.TriCount = std::min(8, OriginalCount - b * 8);

						for (int32 k = 0; k < Block.TriCount; ++k)
						{
							const Triangle& Tri = Triangles[OriginalStart + b * 8 + k];
							Block.AX[k] = Tri.Vertex1.X; Block.AY[k] = Tri.Vertex1.Y; Block.AZ[k] = Tri.Vertex1.Z;
							Block.BX[k] = Tri.Vertex2.X; Block.BY[k] = Tri.Vertex2.Y; Block.BZ[k] = Tri.Vertex2.Z;
							Block.CX[k] = Tri.Vertex3.X; Block.CY[k] = Tri.Vertex3.Y; Block.CZ[k] = Tri.Vertex3.Z;
						}
						// fill the rest with 0
						for (int32 k = Block.TriCount; k < 8; ++k)
						{
							Block.AX[k] = Block.AY[k] = Block.AZ[k] = 0.0f;
							Block.BX[k] = Block.BY[k] = Block.BZ[k] = 0.0f;
							Block.CX[k] = Block.CY[k] = Block.CZ[k] = 0.0f;
						}
						
						TriangleBlocks.push_back(Block);
					}

					Node8.TriangleStart[i] = BlockStart;
					Node8.TriangleCount[i] = NumBlocks;
				}
			}
		}
    }
}

int32 FBVHMesh::CollapseTo8Way(int32 BinaryNodeIndex)
{
    int32 NewNodeIndex = Nodes8Way.size();
    Nodes8Way.push_back(FBVHMeshNode8());

    TArray<int32> BinaryNodesToProcess;
	BinaryNodesToProcess.push_back(BinaryNodeIndex);

    while (BinaryNodesToProcess.size() < 8)
    {
        int32 LargestInternalIndex = -1;
        float MaxSurfaceArea = -1.0f;

        for (int32 i = 0; i < BinaryNodesToProcess.size(); ++i)
        {
            int32 NodeIdx = BinaryNodesToProcess[i];
            const FBVHMeshNode& BinNode = Nodes[NodeIdx];

            if (!BinNode.IsLeaf())
            {
                float SurfaceArea = BinNode.Bounds.GetSurfaceArea();
                if (SurfaceArea > MaxSurfaceArea)
                {
                    LargestInternalIndex = i;
					MaxSurfaceArea = SurfaceArea;
                }
            }
        }

		if (LargestInternalIndex == -1) break;

		int32 ParentToSplit = BinaryNodesToProcess[LargestInternalIndex];
		BinaryNodesToProcess.erase(BinaryNodesToProcess.begin() + LargestInternalIndex);

		BinaryNodesToProcess.push_back(Nodes[ParentToSplit].LeftChild);
		BinaryNodesToProcess.push_back(Nodes[ParentToSplit].RightChild);
    }

	Nodes8Way[NewNodeIndex].ValidChildCount = BinaryNodesToProcess.size();
    for (int32 i = 0; i < Nodes8Way[NewNodeIndex].ValidChildCount; ++i)
    {
		int32 BinNodeIdx = BinaryNodesToProcess[i];
        const FBVHMeshNode& BinNode = Nodes[BinNodeIdx];

        Nodes8Way[NewNodeIndex].ChildMinX[i] = BinNode.Bounds.Min.X;
        Nodes8Way[NewNodeIndex].ChildMinY[i] = BinNode.Bounds.Min.Y;
        Nodes8Way[NewNodeIndex].ChildMinZ[i] = BinNode.Bounds.Min.Z;
        Nodes8Way[NewNodeIndex].ChildMaxX[i] = BinNode.Bounds.Max.X;
        Nodes8Way[NewNodeIndex].ChildMaxY[i] = BinNode.Bounds.Max.Y;
        Nodes8Way[NewNodeIndex].ChildMaxZ[i] = BinNode.Bounds.Max.Z;

        if (BinNode.IsLeaf())
        {
            Nodes8Way[NewNodeIndex].ChildIndices[i] = -1;
            Nodes8Way[NewNodeIndex].TriangleStart[i] = BinNode.startIndex;
            Nodes8Way[NewNodeIndex].TriangleCount[i] = BinNode.endIndex - BinNode.startIndex;
        }
        else
        {
            Nodes8Way[NewNodeIndex].ChildIndices[i] = CollapseTo8Way(BinNodeIdx);
        }
    }
    return NewNodeIndex;
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
    if (Depth >= maxDepth || NumTris <= 16)  // 2→4로 변경
        return CurrentNodeIndex;

    // 3. 분할 축 결정 (가장 긴 축)
    FVector Size = CurrentBounds.Max - CurrentBounds.Min;

	float ParentArea = SurfaceArea(CurrentBounds);
	float BestCost = std::numeric_limits<float>::max();
	int32 BestSplit = -1;
	int BestAxis = -1;

    constexpr int32 NumBuckets = 12;
    struct Bucket { int32 Count = 0; FBoundingBox Bounds; };

    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        if (Size[Axis] < 1e-8f) continue;

        Bucket Buckets[NumBuckets];
		float AxisMin = CurrentBounds.Min[Axis];
        float AxisLen = Size[Axis];

        for (int32 i = Start; i < End; ++i)
        {
            FVector TriCenter = (Triangles[i].Vertex1 + Triangles[i].Vertex2 + Triangles[i].Vertex3) / 3.0f;
            int32 b = static_cast<int32>(NumBuckets * ((TriCenter[Axis] - AxisMin) / AxisLen));
            b = std::clamp(b, 0, NumBuckets - 1);
            Buckets[b].Count++;
            Buckets[b].Bounds.Encapsulate(Triangles[i].Vertex1);
            Buckets[b].Bounds.Encapsulate(Triangles[i].Vertex2);
            Buckets[b].Bounds.Encapsulate(Triangles[i].Vertex3);
		}

        for (int32 split = 1; split < NumBuckets; ++split)
        {
            FBoundingBox LeftBounds;
            int32 LeftCount = 0;
            for (int32 i = 0; i < split; ++i)
            {
                if (Buckets[i].Count == 0) continue;
                LeftBounds.Encapsulate(Buckets[i].Bounds.Min);
                LeftBounds.Encapsulate(Buckets[i].Bounds.Max);
                LeftCount += Buckets[i].Count;
            }

            FBoundingBox RightBounds;
            int32 RightCount = 0;
            for (int32 i = split; i < NumBuckets; ++i)
            {
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
                BestAxis = Axis;
            }
		}
    }

    // 3축 모두 분할에 실패했거나(축 길이 0) 비용 개선이 없는 경우 (Fallback)
    if (BestAxis == -1)
    {
        int32 Mid = Start + NumTris / 2;
        Nodes[CurrentNodeIndex].LeftChild = SplitRecursive(calcBounds(Triangles, Start, Mid), Start, Mid, Depth + 1);
        Nodes[CurrentNodeIndex].RightChild = SplitRecursive(calcBounds(Triangles, Mid, End), Mid, End, Depth + 1);
        return CurrentNodeIndex;
    }

    // 최적의 축과 비율을 기준으로 재정렬
    float AxisMin = CurrentBounds.Min[BestAxis];
    float AxisLen = Size[BestAxis];
    float SplitPos = AxisMin + AxisLen * (static_cast<float>(BestSplit) / NumBuckets);

    auto MidPtr = std::partition(Triangles.begin() + Start, Triangles.begin() + End,
        [BestAxis, SplitPos](const Triangle& Tri)
        {
            float Center = (Tri.Vertex1[BestAxis] + Tri.Vertex2[BestAxis] + Tri.Vertex3[BestAxis]) / 3.0f;
            return Center < SplitPos;
        });

    int32 Mid = static_cast<int32>(MidPtr - Triangles.begin());

    // 예외 처리: 한쪽으로 몰리는 경우 강제 절반 분할
    if (Mid == Start || Mid == End)
    {
        Mid = Start + NumTris / 2;
    }

    FBoundingBox LeftBounds = calcBounds(Triangles, Start, Mid);
    FBoundingBox RightBounds = calcBounds(Triangles, Mid, End);

    int32 LeftIdx = SplitRecursive(LeftBounds, Start, Mid, Depth + 1);
    int32 RightIdx = SplitRecursive(RightBounds, Mid, End, Depth + 1);

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
