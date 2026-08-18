#pragma once
#include "D3D11Common.h"
#include "Types/PlatformTypes.h"
#include "Types/String.h"
#include "Types/Array.h"
#include <unordered_map>
#include <functional>
struct FDepthStencilKey
{
	BOOL DepthEnable;
	D3D11_DEPTH_WRITE_MASK DepthWriteMask;
	D3D11_COMPARISON_FUNC DepthFunc;

	bool operator==(const FDepthStencilKey& Other) const
	{
		return DepthEnable == Other.DepthEnable &&
			DepthWriteMask == Other.DepthWriteMask &&
			DepthFunc == Other.DepthFunc;
	}
};

struct FDepthStencilKeyHash
{
	std::size_t operator()(const FDepthStencilKey& Key) const
	{
		std::size_t Hash = std::hash<BOOL>()(Key.DepthEnable);
		Hash ^= std::hash<int>()(static_cast<int>(Key.DepthWriteMask)) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		Hash ^= std::hash<int>()(static_cast<int>(Key.DepthFunc)) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		return Hash;
	}
};
struct FBlendKey
{
	BOOL BlendEnable;
	D3D11_BLEND SrcBlend;
	D3D11_BLEND DestBlend;
	D3D11_BLEND_OP BlendOp;
	UINT8 RenderTargetWriteMask;

	bool operator==(const FBlendKey& Other) const
	{
		return BlendEnable == Other.BlendEnable &&
			SrcBlend == Other.SrcBlend &&
			DestBlend == Other.DestBlend &&
			BlendOp == Other.BlendOp &&
			RenderTargetWriteMask == Other.RenderTargetWriteMask;
	}
};
struct FBlendKeyHash
{
	std::size_t operator()(const FBlendKey& Key) const
	{
		std::size_t Hash = std::hash<BOOL>()(Key.BlendEnable);
		Hash ^= std::hash<int>()(static_cast<int>(Key.SrcBlend)) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		Hash ^= std::hash<int>()(static_cast<int>(Key.DestBlend)) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		Hash ^= std::hash<int>()(static_cast<int>(Key.BlendOp)) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		Hash ^= std::hash<UINT8>()(Key.RenderTargetWriteMask) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		return Hash;
	}
};
struct FRasterKey
{
	D3D11_FILL_MODE FillMode;
	D3D11_CULL_MODE CullMode;
	bool bFrontCounterClockwise;

	bool operator==(const FRasterKey& Other) const
	{
		return FillMode == Other.FillMode &&
			CullMode == Other.CullMode &&
			bFrontCounterClockwise == Other.bFrontCounterClockwise;
	}
};

struct FRasterKeyHash
{
	std::size_t operator()(const FRasterKey& Key) const
	{
		std::size_t Hash = std::hash<int>()(static_cast<int>(Key.FillMode));
		Hash ^= std::hash<int>()(static_cast<int>(Key.CullMode)) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		Hash ^= std::hash<bool>()(Key.bFrontCounterClockwise) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		return Hash;
	}
};
#include "Scene/SceneTypes.h"

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
	ID3D11RasterizerState* GetRasterizerState(D3D11_FILL_MODE FillMode, D3D11_CULL_MODE CullMode, bool bFrontCounterClockwise) const;
	ID3D11DepthStencilState* GetDepthStencilState(BOOL bDepthEnable, D3D11_DEPTH_WRITE_MASK WriteMask, D3D11_COMPARISON_FUNC DepthFunc) const;
	ID3D11BlendState* GetBlendState(BOOL bBlendEnable, D3D11_BLEND SrcBlend, D3D11_BLEND DestBlend, D3D11_BLEND_OP BlendOp, UINT8 RenderTargetWriteMask) const;
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

	TComPtr<ID3D11Buffer> LastFrameVisibilityBuffer;
	TComPtr<ID3D11ShaderResourceView> LastFrameVisibilitySRV;
	TComPtr<ID3D11Buffer> LastFrameStagingBuffer;

	TComPtr<ID3D11SamplerState> PointSampler;
	void EnsureCullingBufferCapacity(uint32 RequiredCount);
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
	mutable std::unordered_map<FRasterKey, TComPtr<ID3D11RasterizerState>, FRasterKeyHash> RasterCache;
	mutable std::unordered_map<FDepthStencilKey, TComPtr<ID3D11DepthStencilState>, FDepthStencilKeyHash> DepthStencilCache;
	mutable std::unordered_map<FBlendKey, TComPtr<ID3D11BlendState>, FBlendKeyHash> BlendCache;
	D3D11_VIEWPORT Viewport = {};
	uint32 MaxInstanceCapacity = 50000;
	FString AdapterName;
	uint32 AdapterVendorId = 0;
	uint32 AdapterDeviceId = 0;
	uint64 AdapterDedicatedVideoMemoryMB = 0;
	bool bHighPerformancePreferenceApplied = false;
};

