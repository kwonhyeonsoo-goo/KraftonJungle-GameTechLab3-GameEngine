#pragma once
#include <d3d11.h>
#include <CoreMinimal.h>

class CCore;

class CViewport
{
public:
	void Render(CCore* Core);

	void ReadySceneView(uint32 Width, uint32 Height);

private:
	ID3D11RenderTargetView* RenderTargetView = nullptr;

	uint32 OffscreenWidth = 0;
	uint32 OffscreenHeight = 0;
};

