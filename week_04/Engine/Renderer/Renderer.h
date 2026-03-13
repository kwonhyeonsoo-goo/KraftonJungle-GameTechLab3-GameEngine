#pragma once

#include "CoreMinimal.h"
#include "ShaderManager.h"
#include <d3d11.h>

class ENGINE_API CRenderer
{
public:
	CRenderer() = default;
	~CRenderer();

	bool Initialize(HWND Hwnd, int Width, int Height);
	void BeginFrame();
	void DrawTestTriangle();

	void EndFrame();
	void Release();
	bool IsOccluded();
	void OnResize(int NewWidth, int NewHeight);
	bool bSwapChainOccluded = false;

	ID3D11Device* GetDevice() const { return Device; }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }
	ID3D11RenderTargetView* GetRenderTargetView() const {return RenderTargetView;}
	IDXGISwapChain* GetSwapChain() const {return SwapChain;};
private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;
	D3D11_VIEWPORT Viewport = {};
	CShaderManager ShaderManager;
};
