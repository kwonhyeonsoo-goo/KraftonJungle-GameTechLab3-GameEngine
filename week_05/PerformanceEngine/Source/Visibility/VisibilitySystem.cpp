#include "Visibility/VisibilitySystem.h"

#include "Camera/Camera.h"
#include "Scene/Scene.h"

void FVisibilitySystem::Reset()
{
	NextFrameNumber = 1;
}

void FVisibilitySystem::Build(const FScene& InScene, const FCamera& InCamera, FVisibilityResults& OutResults)
{
	(void)InCamera;

	const TArray<FRenderItem>& RenderItems = InScene.GetRenderItems();
	OutResults.FrameNumber = NextFrameNumber++;
	OutResults.VisiblePrimitiveIndices.clear();
	OutResults.VisiblePrimitiveIndices.reserve(RenderItems.size());

	for (uint32 PrimitiveIndex = 0; PrimitiveIndex < static_cast<uint32>(RenderItems.size()); ++PrimitiveIndex)
	{
		OutResults.VisiblePrimitiveIndices.push_back(PrimitiveIndex);
	}
}
