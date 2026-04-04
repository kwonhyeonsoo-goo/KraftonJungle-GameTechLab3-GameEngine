#pragma once
#include "D3D11Common.h"
#include "Types/PlatformTypes.h"
#include "Types/String.h"
#include "Types/Array.h"

struct FInstanceData
{
	DirectX::XMFLOAT4X4 WorldMatrix;
	DirectX::XMFLOAT3 Center;
	float Padding1;
	DirectX::XMFLOAT3 Extents;
	float Padding2;
};

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
	ID3D11ShaderResourceView* GetDepthStencilSRV() const { return DepthStencilSRV.Get(); }

	D3D11_VIEWPORT GetViewport() const { return Viewport; }

	int32 GetViewportWidth() const { return ViewportWidth; }
	int32 GetViewportHeight() const { return ViewportHeight; }
	const FString& GetAdapterName() const { return AdapterName; }
	uint32 GetAdapterVendorId() const { return AdapterVendorId; }
	uint32 GetAdapterDeviceId() const { return AdapterDeviceId; }
	uint64 GetAdapterDedicatedVideoMemoryMB() const { return AdapterDedicatedVideoMemoryMB; }
	bool IsHighPerformancePreferenceApplied() const { return bHighPerformancePreferenceApplied; }

	TComPtr<ID3D11Texture2D> HiZDepthTexture;
	TComPtr<ID3D11ShaderResourceView> HiZFullSRV;
	TArray<TComPtr<ID3D11ShaderResourceView>> HiZDepthSRVs;
	TArray<TComPtr<ID3D11UnorderedAccessView>> HiZDepthUAVs;

	TComPtr<ID3D11ComputeShader> HiZCopyDepthCS;
	TComPtr<ID3D11ComputeShader> HiZBuildMipsCS;
	TComPtr<ID3D11ComputeShader> HiZCullCS;

	TComPtr<ID3D11Buffer> VisibilityBuffer;
	TComPtr<ID3D11UnorderedAccessView> VisibilityUAV;
	TComPtr<ID3D11Buffer> StagingBuffer;

	TComPtr<ID3D11Buffer> InstanceBuffer;
	TComPtr<ID3D11ShaderResourceView> InstanceSRV;

	TComPtr<ID3D11SamplerState> LinearSampler;

private:
	bool CreateBackBufferResources();
	void ReleaseBackBufferResources();

	bool CreateComputeShaders();
	void ReleaseComputeShaders();

	bool CreateVisibilityBuffer();
	void ReleaseVisibilityBuffer();

	void BindBackBuffer();
	void UpdateViewport(int32 InWidth, int32 InHeight);
	void UpdateAdapterInfo(bool bInHighPerformancePreferenceApplied);

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
	TComPtr<ID3D11ShaderResourceView> DepthStencilSRV;

	D3D11_VIEWPORT Viewport = {};

	FString AdapterName;
	uint32 AdapterVendorId = 0;
	uint32 AdapterDeviceId = 0;
	uint64 AdapterDedicatedVideoMemoryMB = 0;
	bool bHighPerformancePreferenceApplied = false;
};

