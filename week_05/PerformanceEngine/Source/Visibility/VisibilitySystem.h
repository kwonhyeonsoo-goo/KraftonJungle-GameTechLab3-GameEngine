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
	// 🚨 피킹 시스템을 위해 새로 추가할 플래그 배열 (Zero-Allocation 용)
	std::vector<uint8_t> VisibleFlags;
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
