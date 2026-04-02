#pragma once
#include <d3d11.h>
#include <CoreMinimal.h>

class FCore;
class FRenderer;
class FViewportClient;

class FViewportLegacy
{
public:
	~FViewportLegacy();

	void Render(HWND Hwnd);
	bool bImageHovered = false;
private:
	void ReleaseLevelView();
	void ReadyLevelView(ID3D11Device* Device, uint32 Width, uint32 Height);

	ID3D11Texture2D* RenderTargetTexture = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;
	ID3D11ShaderResourceView* ShaderResourceView = nullptr;
	ID3D11Texture2D* DepthStencilTexture = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;

	uint32 OffscreenWidth = 0;
	uint32 OffscreenHeight = 0;
};
