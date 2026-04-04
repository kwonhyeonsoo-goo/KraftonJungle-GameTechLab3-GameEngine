#pragma once

#include "BVHNode.h"
#include "Types/Array.h"
#include "Types/PlatformTypes.h"
#include "Math/BoundingBox.h"
#include "Math/Vector.h"

struct FBVHObjectInfo
{
	int32 ObjectIndex;
	FVector Center;
	FBoundingBox Box;
};

class FBVH
{
public:
	void Build(const TArray<FBoundingBox>& ObjectBoxes);

private:
	TArray<FBVHNode> Nodes;
	TArray<int32> OrderedIndices;

	int32 BuildRecursive(TArray<FBVHObjectInfo>& Infos, int32 Start, int32 End);
};
