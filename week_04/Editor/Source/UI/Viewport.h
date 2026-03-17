#pragma once
#include <d3d11.h>
#include <CoreMinimal.h>

class CCore;

class CViewport
{
public:
	void Render(CCore* Core);

private:
	ID3D11RenderTargetView* RenderTargetView;
};

