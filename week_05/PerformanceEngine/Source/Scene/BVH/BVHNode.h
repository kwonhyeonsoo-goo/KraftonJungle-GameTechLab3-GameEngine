#pragma once

#include "Math/BoundingBox.h"
#include "Types/PlatformTypes.h"

struct FBVHNode
{
	FBoundingBox Bounds;
	int32 LeftChild = -1;
	int32 RightChild = -1;
	int32 ObjectIndicesStart = -1;
	int32 ObjectCount = 0;
	
	inline bool IsLeaf() const { return LeftChild == -1; }
};
