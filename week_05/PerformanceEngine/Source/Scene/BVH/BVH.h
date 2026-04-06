#pragma once

#include "BVHNode.h"
#include "Types/Array.h"
#include "Types/PlatformTypes.h"
#include "Math/BoundingBox.h"
#include "Math/Frustum.h"
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
	void Reset();
	void Build(const TArray<FBoundingBox>& ObjectBoxes);

	void GetVisibleObjects(const FFrustum& InFrustum, const FVector& CameraPos, TArray<int32>& OutVisibleObjectIndices) const;

private:
	TArray<FBVHNode> Nodes;
	TArray<int32> OrderedIndices;

	int32 BuildRecursive(TArray<FBVHObjectInfo>& Infos, int32 Start, int32 End);
};
