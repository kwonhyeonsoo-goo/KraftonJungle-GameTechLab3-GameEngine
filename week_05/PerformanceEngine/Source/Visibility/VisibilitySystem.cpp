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

	// 디버그 출력용

	OutResults.VisiblePrimitiveIndices.clear();
	OutResults.VisiblePrimitiveIndices.reserve(InScene.GetPrimitiveCount());

	for (uint32 PrimitiveIndex = 0; PrimitiveIndex < static_cast<uint32>(InScene.GetPrimitiveCount()); ++PrimitiveIndex)
	{
		OutResults.VisiblePrimitiveIndices.push_back(PrimitiveIndex);
	}
}