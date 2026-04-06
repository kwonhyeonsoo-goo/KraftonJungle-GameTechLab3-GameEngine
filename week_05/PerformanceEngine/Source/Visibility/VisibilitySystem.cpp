#include "Visibility/VisibilitySystem.h"

#include "Camera/Camera.h"
#include "Scene/Scene.h"
#include "StaticMesh/StaticMesh.h"
#include "Math/MathUtility.h"
#include "Math/Vector.h"

#include <algorithm>

void FVisibilitySystem::Reset()
{
	NextFrameNumber = 1;

	BVH.Reset();
}

void FVisibilitySystem::Build(const FScene& InScene, const FCamera& InCamera, FVisibilityResults& OutResults)
{
	OutResults.FrameNumber = NextFrameNumber++;

	ViewFrustum.Update(InCamera.GetViewMatrix() * InCamera.GetProjectionMatrix());

	OutResults.VisiblePrimitiveIndices.clear();
	BVH.GetVisibleObjects(ViewFrustum, InCamera.GetLocation(), OutResults.VisiblePrimitiveIndices);

	//피킹을 위한 플래그 캐싱 로직
	const size_t TotalPrimitives = InScene.GetPrimitiveCount();

	// 1. 크기가 부족하면 한 번만 늘려줍니다 (재할당 방지)
	if (OutResults.VisibleFlags.size() < TotalPrimitives)
	{
		OutResults.VisibleFlags.resize(TotalPrimitives, 0);
	}

	// 2. 가장 빠른 속도로 전체 배열을 0으로 초기화 (메모리 통째로 밀기)
	if (TotalPrimitives > 0)
	{
		std::fill(OutResults.VisibleFlags.begin(), OutResults.VisibleFlags.end(), 0);
	}

	// 3. 보이는 오브젝트의 인덱스만 1로 켭니다.
	for (uint32 Idx : OutResults.VisiblePrimitiveIndices)
	{
		OutResults.VisibleFlags[Idx] = 1;
	}
}

void FVisibilitySystem::BuildBVH(const FScene& InScene)
{
	TArray<FScenePrimitiveRuntimeData> Primitives = InScene.GetPrimitiveRuntimeData();
	TArray<FBoundingBox> PrimitiveBoxes;
	PrimitiveBoxes.reserve(Primitives.size());

	for (const FScenePrimitiveRuntimeData& Primitive : Primitives)
	{
		PrimitiveBoxes.push_back(Primitive.WorldBounds);
	}

	BVH.Build(PrimitiveBoxes);
}
