#include "D3D11RHI.h"

#include <algorithm>
#include <sstream>
#include <vector>

namespace
{
	constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	constexpr float ClearColor[] = { 0.08f, 0.10f, 0.14f, 1.0f };

	struct FAdapterCandidate
	{
		TComPtr<IDXGIAdapter1> Adapter;
		DXGI_ADAPTER_DESC1 Desc = {};
		bool bHighPerformancePreference = false;
	};

	bool IsSameAdapterLuid(const LUID& InA, const LUID& InB)
	{
		return InA.LowPart == InB.LowPart && InA.HighPart == InB.HighPart;
	}

	bool IsSoftwareAdapter(const DXGI_ADAPTER_DESC1& InDesc)
	{
		return (InDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
	}

	bool ContainsAdapter(const std::vector<FAdapterCandidate>& InCandidates, const DXGI_ADAPTER_DESC1& InDesc)
	{
		return std::any_of(InCandidates.begin(), InCandidates.end(), [&](const FAdapterCandidate& Candidate)
		{
			return IsSameAdapterLuid(Candidate.Desc.AdapterLuid, InDesc.AdapterLuid);
		});
	}

	void AppendHardwareAdapter(std::vector<FAdapterCandidate>& OutCandidates, IDXGIAdapter1* InAdapter, bool bInHighPerformancePreference)
	{
		if (InAdapter == nullptr)
		{
			return;
		}

		DXGI_ADAPTER_DESC1 Desc = {};
		if (FAILED(InAdapter->GetDesc1(&Desc)) || IsSoftwareAdapter(Desc) || ContainsAdapter(OutCandidates, Desc))
		{
			return;
		}

		FAdapterCandidate Candidate = {};
		Candidate.Adapter = InAdapter;
		Candidate.Desc = Desc;
		Candidate.bHighPerformancePreference = bInHighPerformancePreference;
		OutCandidates.push_back(std::move(Candidate));
	}

	std::vector<FAdapterCandidate> CollectAdapterCandidates()
	{
		TComPtr<IDXGIFactory1> Factory;
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(Factory.GetAddressOf()))))
		{
			return {};
		}

		std::vector<FAdapterCandidate> Candidates;

		TComPtr<IDXGIFactory6> Factory6;
		if (SUCCEEDED(Factory.As(&Factory6)) && Factory6)
		{
			for (UINT AdapterIndex = 0;; ++AdapterIndex)
			{
				TComPtr<IDXGIAdapter1> Adapter;
				const HRESULT Result = Factory6->EnumAdapterByGpuPreference(
					AdapterIndex,
					DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
					IID_PPV_ARGS(Adapter.GetAddressOf()));
				if (Result == DXGI_ERROR_NOT_FOUND)
				{
					break;
				}

				if (SUCCEEDED(Result))
				{
					AppendHardwareAdapter(Candidates, Adapter.Get(), true);
				}
			}
		}

		std::vector<FAdapterCandidate> FallbackCandidates;
		for (UINT AdapterIndex = 0;; ++AdapterIndex)
		{
			TComPtr<IDXGIAdapter1> Adapter;
			const HRESULT Result = Factory->EnumAdapters1(AdapterIndex, Adapter.GetAddressOf());
			if (Result == DXGI_ERROR_NOT_FOUND)
			{
				break;
			}

			if (SUCCEEDED(Result))
			{
				AppendHardwareAdapter(FallbackCandidates, Adapter.Get(), false);
			}
		}

		std::sort(FallbackCandidates.begin(), FallbackCandidates.end(), [](const FAdapterCandidate& A, const FAdapterCandidate& B)
		{
			return A.Desc.DedicatedVideoMemory > B.Desc.DedicatedVideoMemory;
		});

		for (const FAdapterCandidate& Candidate : FallbackCandidates)
		{
			if (!ContainsAdapter(Candidates, Candidate.Desc))
			{
				Candidates.push_back(Candidate);
			}
		}

		return Candidates;
	}

	FString WideToUtf8(const wchar_t* InText)
	{
		if (InText == nullptr || InText[0] == L'\0')
		{
			return {};
		}

		const int RequiredChars = WideCharToMultiByte(CP_UTF8, 0, InText, -1, nullptr, 0, nullptr, nullptr);
		if (RequiredChars <= 1)
		{
			return {};
		}

		std::vector<char> Buffer(static_cast<size_t>(RequiredChars), '\0');
		if (WideCharToMultiByte(CP_UTF8, 0, InText, -1, Buffer.data(), RequiredChars, nullptr, nullptr) <= 0)
		{
			return {};
		}

		return FString(Buffer.data());
	}
}

bool FD3D11RHI::Initialize(HWND InWindowHandle)
{
	if (InWindowHandle == nullptr)
	{
		return false;
	}

	WindowHandle = InWindowHandle;

	RECT ClientRect = {};
	if (!GetClientRect(WindowHandle, &ClientRect))
	{
		return false;
	}

	const int32 ClientWidth = std::max<int32>(ClientRect.right - ClientRect.left, 1);
	const int32 ClientHeight = std::max<int32>(ClientRect.bottom - ClientRect.top, 1);

	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
	SwapChainDesc.BufferCount = 1;
	SwapChainDesc.BufferDesc.Width = static_cast<UINT>(ClientWidth);
	SwapChainDesc.BufferDesc.Height = static_cast<UINT>(ClientHeight);
	SwapChainDesc.BufferDesc.Format = BackBufferFormat;
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.OutputWindow = WindowHandle;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.Windowed = TRUE;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	const D3D_FEATURE_LEVEL RequestedFeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};

	D3D_FEATURE_LEVEL CreatedFeatureLevel = D3D_FEATURE_LEVEL_11_0;

	auto CreateDeviceAndSwapChain = [&](UINT InFlags, const D3D_FEATURE_LEVEL* InFeatureLevels, UINT InFeatureLevelCount)
	{
		SwapChain.Reset();
		Device.Reset();
		DeviceContext.Reset();

		return D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			InFlags,
			InFeatureLevels,
			InFeatureLevelCount,
			D3D11_SDK_VERSION,
			&SwapChainDesc,
			SwapChain.GetAddressOf(),
			Device.GetAddressOf(),
			&CreatedFeatureLevel,
			DeviceContext.GetAddressOf());
	};

	auto CreateDeviceAndSwapChainForAdapter = [&](IDXGIAdapter* InAdapter, D3D_DRIVER_TYPE InDriverType, UINT InFlags, const D3D_FEATURE_LEVEL* InFeatureLevels, UINT InFeatureLevelCount)
	{
		SwapChain.Reset();
		Device.Reset();
		DeviceContext.Reset();

		return D3D11CreateDeviceAndSwapChain(
			InAdapter,
			InDriverType,
			nullptr,
			InFlags,
			InFeatureLevels,
			InFeatureLevelCount,
			D3D11_SDK_VERSION,
			&SwapChainDesc,
			SwapChain.GetAddressOf(),
			Device.GetAddressOf(),
			&CreatedFeatureLevel,
			DeviceContext.GetAddressOf());
	};

	UINT CreateFlags = 0;
#if defined(_DEBUG)
	CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	auto AttemptCreateDeviceAndSwapChain = [&](IDXGIAdapter* InAdapter, D3D_DRIVER_TYPE InDriverType)
	{
		HRESULT Result = CreateDeviceAndSwapChainForAdapter(
			InAdapter,
			InDriverType,
			CreateFlags,
			RequestedFeatureLevels,
			_countof(RequestedFeatureLevels));
		if (Result == E_INVALIDARG)
		{
			Result = CreateDeviceAndSwapChainForAdapter(
				InAdapter,
				InDriverType,
				CreateFlags,
				RequestedFeatureLevels + 1,
				_countof(RequestedFeatureLevels) - 1);
		}
#if defined(_DEBUG)
		if (FAILED(Result) && (CreateFlags & D3D11_CREATE_DEVICE_DEBUG) != 0)
		{
			const UINT FallbackFlags = CreateFlags & ~D3D11_CREATE_DEVICE_DEBUG;
			Result = CreateDeviceAndSwapChainForAdapter(
				InAdapter,
				InDriverType,
				FallbackFlags,
				RequestedFeatureLevels,
				_countof(RequestedFeatureLevels));
			if (Result == E_INVALIDARG)
			{
				Result = CreateDeviceAndSwapChainForAdapter(
					InAdapter,
					InDriverType,
					FallbackFlags,
					RequestedFeatureLevels + 1,
					_countof(RequestedFeatureLevels) - 1);
			}
		}
#endif
		return Result;
	};

	HRESULT Result = E_FAIL;
	bool bUsedHighPerformancePreference = false;

	for (const FAdapterCandidate& Candidate : CollectAdapterCandidates())
	{
		Result = AttemptCreateDeviceAndSwapChain(Candidate.Adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN);
		if (SUCCEEDED(Result))
		{
			bUsedHighPerformancePreference = Candidate.bHighPerformancePreference;
			break;
		}
	}

	if (FAILED(Result))
	{
		Result = CreateDeviceAndSwapChain(CreateFlags, RequestedFeatureLevels, _countof(RequestedFeatureLevels));
		if (Result == E_INVALIDARG)
		{
			Result = CreateDeviceAndSwapChain(CreateFlags, RequestedFeatureLevels + 1, _countof(RequestedFeatureLevels) - 1);
		}
#if defined(_DEBUG)
		if (FAILED(Result) && (CreateFlags & D3D11_CREATE_DEVICE_DEBUG) != 0)
		{
			CreateFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
			Result = CreateDeviceAndSwapChain(CreateFlags, RequestedFeatureLevels, _countof(RequestedFeatureLevels));
			if (Result == E_INVALIDARG)
			{
				Result = CreateDeviceAndSwapChain(CreateFlags, RequestedFeatureLevels + 1, _countof(RequestedFeatureLevels) - 1);
			}
		}
#endif
	}

	if (FAILED(Result))
	{
		Shutdown();
		return false;
	}

	UpdateAdapterInfo(bUsedHighPerformancePreference);
	UpdateViewport(ClientWidth, ClientHeight);

	if (!CreateBackBufferResources())
	{
		Shutdown();
		return false;
	}

	BindBackBuffer();
	return true;
}

void FD3D11RHI::Shutdown()
{
	if (DeviceContext)
	{
		DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
		DeviceContext->ClearState();
		DeviceContext->Flush();
	}

	ReleaseBackBufferResources();
	SwapChain.Reset();
	DeviceContext.Reset();
	Device.Reset();
	AdapterName.clear();
	AdapterVendorId = 0;
	AdapterDeviceId = 0;
	AdapterDedicatedVideoMemoryMB = 0;
	bHighPerformancePreferenceApplied = false;

	WindowHandle = nullptr;
	UpdateViewport(0, 0);
}

void FD3D11RHI::BeginFrame()
{
	if (!DeviceContext || !BackBufferRTV || !DepthStencilView)
	{
		return;
	}

	BindBackBuffer();
	DeviceContext->ClearRenderTargetView(BackBufferRTV.Get(), ClearColor);
	DeviceContext->ClearDepthStencilView(DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void FD3D11RHI::EndFrame()
{
	if (!SwapChain)
	{
		return;
	}

	SwapChain->Present(bVSyncEnabled ? 1u : 0u, 0);
}

bool FD3D11RHI::Resize(int32 InWidth, int32 InHeight)
{
	if (!SwapChain || !DeviceContext || InWidth <= 0 || InHeight <= 0)
	{
		return false;
	}

	DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	DeviceContext->ClearState();
	DeviceContext->Flush();

	ReleaseBackBufferResources();

	const HRESULT Result = SwapChain->ResizeBuffers(
		0,
		static_cast<UINT>(InWidth),
		static_cast<UINT>(InHeight),
		DXGI_FORMAT_UNKNOWN,
		0);

	if (FAILED(Result))
	{
		return false;
	}

	UpdateViewport(InWidth, InHeight);

	if (!CreateBackBufferResources())
	{
		return false;
	}

	BindBackBuffer();
	return true;
}

bool FD3D11RHI::CreateBackBufferResources()
{
	if (!Device || !SwapChain || ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		return false;
	}

	HRESULT Result = SwapChain->GetBuffer(0, IID_PPV_ARGS(BackBufferTexture.GetAddressOf()));
	if (FAILED(Result))
	{
		return false;
	}

	Result = Device->CreateRenderTargetView(BackBufferTexture.Get(), nullptr, BackBufferRTV.GetAddressOf());
	if (FAILED(Result))
	{
		ReleaseBackBufferResources();
		return false;
	}

	D3D11_TEXTURE2D_DESC DepthStencilDesc = {};
	DepthStencilDesc.Width = static_cast<UINT>(ViewportWidth);
	DepthStencilDesc.Height = static_cast<UINT>(ViewportHeight);
	DepthStencilDesc.MipLevels = 1;
	DepthStencilDesc.ArraySize = 1;
	DepthStencilDesc.Format = DepthStencilFormat;
	DepthStencilDesc.SampleDesc.Count = 1;
	DepthStencilDesc.SampleDesc.Quality = 0;
	DepthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	Result = Device->CreateTexture2D(&DepthStencilDesc, nullptr, DepthStencilBuffer.GetAddressOf());
	if (FAILED(Result))
	{
		ReleaseBackBufferResources();
		return false;
	}

	Result = Device->CreateDepthStencilView(DepthStencilBuffer.Get(), nullptr, DepthStencilView.GetAddressOf());
	if (FAILED(Result))
	{
		ReleaseBackBufferResources();
		return false;
	}

	return true;
}

void FD3D11RHI::ReleaseBackBufferResources()
{
	DepthStencilView.Reset();
	DepthStencilBuffer.Reset();
	BackBufferRTV.Reset();
	BackBufferTexture.Reset();
}

void FD3D11RHI::BindBackBuffer()
{
	if (!DeviceContext || !BackBufferRTV || !DepthStencilView)
	{
		return;
	}

	ID3D11RenderTargetView* RenderTargets[] = { BackBufferRTV.Get() };
	DeviceContext->OMSetRenderTargets(1, RenderTargets, DepthStencilView.Get());
	DeviceContext->RSSetViewports(1, &Viewport);
}

void FD3D11RHI::UpdateViewport(int32 InWidth, int32 InHeight)
{
	ViewportWidth = InWidth;
	ViewportHeight = InHeight;

	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = static_cast<float>(std::max(InWidth, 0));
	Viewport.Height = static_cast<float>(std::max(InHeight, 0));
	Viewport.MinDepth = D3D11_MIN_DEPTH;
	Viewport.MaxDepth = D3D11_MAX_DEPTH;
}

void FD3D11RHI::UpdateAdapterInfo(bool bInHighPerformancePreferenceApplied)
{
	AdapterName.clear();
	AdapterVendorId = 0;
	AdapterDeviceId = 0;
	AdapterDedicatedVideoMemoryMB = 0;
	bHighPerformancePreferenceApplied = bInHighPerformancePreferenceApplied;

	if (!Device)
	{
		return;
	}

	TComPtr<IDXGIDevice> DxgiDevice;
	if (FAILED(Device.As(&DxgiDevice)) || !DxgiDevice)
	{
		return;
	}

	TComPtr<IDXGIAdapter> DxgiAdapter;
	if (FAILED(DxgiDevice->GetAdapter(DxgiAdapter.GetAddressOf())) || !DxgiAdapter)
	{
		return;
	}

	TComPtr<IDXGIAdapter1> DxgiAdapter1;
	if (FAILED(DxgiAdapter.As(&DxgiAdapter1)) || !DxgiAdapter1)
	{
		return;
	}

	DXGI_ADAPTER_DESC1 Desc = {};
	if (FAILED(DxgiAdapter1->GetDesc1(&Desc)))
	{
		return;
	}

	AdapterName = WideToUtf8(Desc.Description);
	AdapterVendorId = Desc.VendorId;
	AdapterDeviceId = Desc.DeviceId;
	AdapterDedicatedVideoMemoryMB = static_cast<uint64>(Desc.DedicatedVideoMemory / (1024ull * 1024ull));

	std::ostringstream LogStream;
	LogStream << "[D3D11RHI] Selected adapter: "
		<< (AdapterName.empty() ? "Unknown" : AdapterName)
		<< " | VRAM: " << AdapterDedicatedVideoMemoryMB << " MB";
	if (bHighPerformancePreferenceApplied)
	{
		LogStream << " | HighPerformancePreference";
	}
	LogStream << '\n';
	OutputDebugStringA(LogStream.str().c_str());
}
