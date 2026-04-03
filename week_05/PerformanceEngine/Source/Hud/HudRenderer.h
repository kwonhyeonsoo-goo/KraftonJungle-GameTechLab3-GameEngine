#pragma once

#include <memory>

#include "Graphics/D3D11/D3D11Common.h"

class FCamera;
class FD3D11RHI;
class FScene;
class FStatsSystem;
struct FPickState;

class FHudRenderer
{
public:
	FHudRenderer();
	~FHudRenderer();

	bool Initialize(FD3D11RHI& InRHI);
	void Shutdown();
	void Render(const FD3D11RHI& InRHI, const FCamera& InCamera, const FScene& InScene, const FStatsSystem& InStatsSystem, const FPickState& InPickState);

private:
	struct FResources;
	std::unique_ptr<FResources> Resources;
};
