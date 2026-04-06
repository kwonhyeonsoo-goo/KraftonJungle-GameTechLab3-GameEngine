#pragma once

#include "BVHMeshNode.h"
#include "Types/Array.h"
#include "Types/PlatformTypes.h"
#include "Math/BoundingBox.h"
#include "Math/Vector.h"
#include "Math/Vector2.h"

struct Triangle
{
	FVector Vertex1, Vertex2, Vertex3;
	FVector2 Texcoord1, Texcoord2, Texcoord3;
};

struct FBVHMeshNode
{
	FBoundingBox Bounds;

	int32 startIndex;
	int32 endIndex;

	int32 LeftChild = -1;
	int32 RightChild = -1;


	inline bool IsLeaf() const { return LeftChild == -1; }
};

struct FBVHInfo
{
	int32 ObjectIndex;
	FVector Center;
	FBoundingBox Box;
};

struct FTriangleBlock8
{
	alignas(32) float AX[8] = {0}, AY[8] = {0}, AZ[8] = {0};
	alignas(32) float BX[8] = {0}, BY[8] = {0}, BZ[8] = {0};
	alignas(32) float CX[8] = {0}, CY[8] = {0}, CZ[8] = {0};
	int32 TriCount = 0;
};

struct FBVHMeshNode8
{
	alignas(32) float ChildMinX[8];
	alignas(32) float ChildMinY[8];
	alignas(32) float ChildMinZ[8];

	alignas(32) float ChildMaxX[8];
	alignas(32) float ChildMaxY[8];
	alignas(32) float ChildMaxZ[8];

	int32 ChildIndices[8];      // 내부 노드일 경우 다음 8-Way 노드의 인덱스, 단말일 경우 -1
	int32 TriangleStart[8];     // 단말 노드(Leaf)일 경우 삼각형 시작 인덱스
	int32 TriangleCount[8];     // 단말 노드(Leaf)일 경우 삼각형 개수

	int32 ValidChildCount = 0;  // 실제로 채워진 자식의 개수 (1 ~ 8)
};


class FBVHMesh
{
public:
	void Build(const FBoundingBox& ObjectBoxe, TArray<Triangle> triangles);

	int32 CollapseTo8Way(int32 BinaryNodeIndex);

	const TArray<FBVHMeshNode>& GetNodes() const { return Nodes; }
	const TArray<Triangle>& GetTriangles() const { return Triangles; }
	const TArray<FBVHMeshNode8>& GetNodes8Way() const { return Nodes8Way; }
	const TArray<FTriangleBlock8>& GetTriangleBlocks() const { return TriangleBlocks; }
	const static int32 maxDepth = 128;
private:
	TArray<FBVHMeshNode> Nodes;
	TArray<FBVHMeshNode8> Nodes8Way;
	TArray<FTriangleBlock8> TriangleBlocks;
	FBVHMeshNode Root;

	TArray<int32> OrderedIndices;

	int32 SplitRecursive(const FBoundingBox& CurrentBounds, int32 Start, int32 End, int32 Depth);


	TArray<Triangle> Triangles;

	FBoundingBox calcBounds(const TArray<Triangle>& triangles, int start, int end);
};
