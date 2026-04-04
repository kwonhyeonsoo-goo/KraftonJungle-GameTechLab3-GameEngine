#include "Visibility/VisibilitySystem.h"

#include "Camera/Camera.h"
#include "Scene/Scene.h"
#include "StaticMesh/StaticMesh.h"
#include "Math/MathUtility.h"
#include "Math/Vector.h"

void FVisibilitySystem::Reset()
{
	NextFrameNumber = 1;
}

void FVisibilitySystem::Build(const FScene& InScene, const FCamera& InCamera, FVisibilityResults& OutResults)
{
	OutResults.FrameNumber = NextFrameNumber++;

	const TArray<FScenePrimitiveRuntimeData>& PrimitiveRuntimeData = InScene.GetPrimitiveRuntimeData();
	TArray<FBoundingBox> BoundingBoxes;
	BoundingBoxes.reserve(InScene.GetPrimitiveCount());

	for (const FScenePrimitiveRuntimeData& PrimitiveData : PrimitiveRuntimeData)
	{
		BoundingBoxes.push_back(PrimitiveData.WorldBounds);
	}

	ViewFrustum.Update(InCamera.GetViewMatrix() * InCamera.GetProjectionMatrix());

	OutResults.VisiblePrimitiveIndices.clear();
	BVH.GetVisibleObjects(ViewFrustum, InCamera.GetLocation(), BoundingBoxes, OutResults.VisiblePrimitiveIndices);
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
