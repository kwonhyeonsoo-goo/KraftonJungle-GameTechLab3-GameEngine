#pragma once

#include <memory>

#include "Graphics/D3D11/D3D11Common.h"
#include "Types/Array.h"
#include "Types/PlatformTypes.h"

class FCamera;
class FD3D11RHI;
class FScene;
struct FPickState;
struct FVisibilityResults;

class FSceneRenderer
{
public:
	FSceneRenderer();
	~FSceneRenderer();

	bool Initialize(FD3D11RHI& InRHI);
	void Shutdown();
	void Render(const FD3D11RHI& InRHI, const FScene& InScene, const FCamera& InCamera, const FVisibilityResults& InVisibilityResults, const FPickState& InPickState);

private:
	void Prepare(const FD3D11RHI& InRHI, const FScene& InScene, const FCamera& InCamera);
	void RenderPrimitives(const FD3D11RHI& InRHI, const FScene& InScene, const FCamera& InCamera, const TArray<int32>& VisibleIndices, const FPickState& InPickState);
	void BuildHiZMipChain(const FD3D11RHI& InRHI, const FScene& InScene);

public:
	static uint32 DrawCallCount;

private:
	struct FResources;
	std::unique_ptr<FResources> Resources;
};
