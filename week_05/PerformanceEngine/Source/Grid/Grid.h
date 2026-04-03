#pragma once

#include <memory>

class FCamera;
class FD3D11RHI;

class FGrid
{
public:
	FGrid();
	~FGrid();

	FGrid(const FGrid&) = delete;
	FGrid(FGrid&&) = delete;
	FGrid& operator=(const FGrid&) = delete;
	FGrid& operator=(FGrid&&) = delete;

	bool Initialize(const FD3D11RHI& InRHI);
	void Release();

	void Render(const FD3D11RHI& InRHI, const FCamera& InCamera);

private:
	struct FResources;
	std::unique_ptr<FResources> Resources;
};
