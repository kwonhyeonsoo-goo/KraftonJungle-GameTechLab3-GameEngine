#pragma once

#include "CoreMinimal.h"
#include <d3d11.h>

class ENGINE_API CRenderer
{
public:
	CRenderer() = default;
	~CRenderer();

	bool Initialize(HWND Hwnd, int Width, int Height);
	void BeginFrame();
	void EndFrame();
	void Release();

	ID3D11Device* GetDevice() const { return Device; }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;
	D3D11_VIEWPORT Viewport = {};
};
