#pragma once
#include "D3D11Common.h"
#include "Types/PlatformTypes.h"

class FD3D11RHI
{
public:
	FD3D11RHI() = default;
	~FD3D11RHI() = default;

	bool Initialize(HWND InWindowHandle);
	void Shutdown();

	void BeginFrame();
	void EndFrame();

	bool Resize(int32 InWidth, int32 InHeight);

	ID3D11Device* GetDevice() const { return Device.Get(); }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext.Get(); }
	IDXGISwapChain* GetSwapChain() const { return SwapChain.Get(); }

	ID3D11RenderTargetView* GetBackBufferRTV() const { return BackBufferRTV.Get(); }
	ID3D11Texture2D* GetDepthStencilBuffer() const { return DepthStencilBuffer.Get(); }
	ID3D11DepthStencilView* GetDepthStencilView() const { return DepthStencilView.Get(); }

	D3D11_VIEWPORT GetViewport() const { return Viewport; }

	int32 GetViewportWidth() const { return ViewportWidth; }
	int32 GetViewportHeight() const { return ViewportHeight; }


private:
	bool CreateBackBufferResources();
	void ReleaseBackBufferResources();
	void BindBackBuffer();
	void UpdateViewport(int32 InWidth, int32 InHeight);

private:
	HWND WindowHandle = nullptr;

	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;

	bool bVSyncEnabled = false;

	TComPtr<ID3D11Device>        Device;
	TComPtr<ID3D11DeviceContext> DeviceContext;
	TComPtr<IDXGISwapChain>      SwapChain;

	TComPtr<ID3D11Texture2D>        BackBufferTexture;
	TComPtr<ID3D11RenderTargetView> BackBufferRTV;

	TComPtr<ID3D11Texture2D>        DepthStencilBuffer;
	TComPtr<ID3D11DepthStencilView> DepthStencilView;

	D3D11_VIEWPORT Viewport = {};
};

