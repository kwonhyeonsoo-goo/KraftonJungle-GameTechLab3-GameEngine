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


class FBVHMesh
{
public:
	void Build(const FBoundingBox& ObjectBoxe, TArray<Triangle> triangles);
	const TArray<FBVHMeshNode>& GetNodes() const { return Nodes; }
	const TArray<Triangle>& GetTriangles() const { return Triangles; }
	const static int32 maxDepth = 128;
private:
	TArray<FBVHMeshNode> Nodes;
	FBVHMeshNode Root;

	TArray<int32> OrderedIndices;

	int32 SplitRecursive(const FBoundingBox& CurrentBounds, int32 Start, int32 End, int32 Depth);


	TArray<Triangle> Triangles;

	FBoundingBox calcBounds(const TArray<Triangle>& triangles, int start, int end);
};
