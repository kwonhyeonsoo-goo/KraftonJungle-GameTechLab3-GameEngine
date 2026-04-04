#pragma once

#include "Math/Frustum.h"
#include "Math/Vector.h"
#include "Scene/BVH/BVH.h"
#include "Types/Array.h"
#include "Types/PlatformTypes.h"

class FCamera;
class FScene;

struct FVisibilityResults
{
	uint64 FrameNumber = 0;
	TArray<int32> VisiblePrimitiveIndices;
};

class FVisibilitySystem
{
public:
	void Reset();
	void Build(const FScene& InScene, const FCamera& InCamera, FVisibilityResults& OutResults);

	void BuildBVH(const FScene& InScene);

	FBVH& GetBVH() { return BVH; }


private:
	FFrustum ViewFrustum;
	FBVH BVH;

	uint64 NextFrameNumber = 1;
};
