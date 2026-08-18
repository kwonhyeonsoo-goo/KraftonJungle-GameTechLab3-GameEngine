#pragma once
#include "../CoreMinimal.h"
#include "d3d11.h"
#include "Core/ShowFlags.h"


struct FViewportInfo {
	float TopLeftX, TopLeftY, Width, Height;
	float MinDepth, MaxDepth;
	int32 ClientPosX, ClientPosY;
	ID3D11RenderTargetView* RTV;
	ID3D11DepthStencilView* DSV;
};