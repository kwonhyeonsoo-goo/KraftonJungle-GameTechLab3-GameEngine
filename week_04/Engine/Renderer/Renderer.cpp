#include "Renderer.h"
#include "ShaderType.h"
#include "Shader.h"
#include "ShaderMap.h"
#include <dxgi1_3.h>
#include "Primitive/PrimitiveBase.h"
#include <cassert>
#include <algorithm>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")



CRenderer::~CRenderer()
{
	Release();
}

void CRenderer::SetGUICallbacks(
	FGUICallback InInit,
	FGUICallback InShutdown,
	FGUICallback InNewFrame,
	FGUICallback InRender,
	FGUICallback InPostPresent)
{
	GUIInit = std::move(InInit);
	GUIShutdown = std::move(InShutdown);
	GUINewFrame = std::move(InNewFrame);
	GUIRender = std::move(InRender);
	GUIPostPresent = std::move(InPostPresent);

	if (GUIInit)
	{
		GUIInit();
	}
}

void CRenderer::SetGUIUpdateCallback(FGUICallback InUpdate)
{
	GUIUpdate = std::move(InUpdate);
}

bool CRenderer::Initialize(HWND InHwnd, int Width, int Height)
{
	Hwnd = InHwnd;
	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL featureLevels = D3D_FEATURE_LEVEL_11_0;

	HRESULT hr = D3D11CreateDevice(
		0, D3D_DRIVER_TYPE_HARDWARE, 0,
		createDeviceFlags, 0, 0,
		D3D11_SDK_VERSION, &Device, &featureLevels,
		&DeviceContext
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"D3D11CreateDevice Failed.", 0, 0);
		return false;
	}
	if (featureLevels != D3D_FEATURE_LEVEL_11_0)
	{
		MessageBox(0, L"Direct3D Feature Level 11 unsupported", 0, 0);
		return false;
	}
	UINT m4xMsaaQuality;
	hr = Device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality);
	assert(m4xMsaaQuality > 0);

	IDXGIDevice* dxgiDevice = nullptr;
	hr = Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	if (SUCCEEDED(hr))
	{
		IDXGIAdapter* dxgiAdapter = nullptr;
		hr = dxgiDevice->GetAdapter(&dxgiAdapter);
		if (SUCCEEDED(hr))
		{
			IDXGIFactory2* dxgiFactory = nullptr;
			hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgiFactory);

			if (SUCCEEDED(hr))
			{
				DXGI_SWAP_CHAIN_DESC1 sd = {};
				sd.Width = Width;
				sd.Height = Height;
				sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				sd.SampleDesc.Count = 1;
				sd.SampleDesc.Quality = 0;
				sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				sd.BufferCount = 2;
				sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

				IDXGISwapChain1* swapChain1 = nullptr;
				hr = dxgiFactory->CreateSwapChainForHwnd(
					Device, Hwnd, &sd, nullptr, nullptr, &swapChain1
				);

				if (SUCCEEDED(hr))
				{
					swapChain1->QueryInterface(__uuidof(IDXGISwapChain), (void**)&SwapChain);
					swapChain1->Release();
				}

				dxgiFactory->Release();
			}
			dxgiAdapter->Release();
		}
		dxgiDevice->Release();
	}
	if (!SwapChain)
	{
		MessageBox(0, L"SwapChain creation Failed.", 0, 0);
		return false;
	}
	ID3D11Texture2D* BackBuffer = nullptr;
	hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
	if (FAILED(hr))
	{
		MessageBox(0, L"GetBuffer Failed.", 0, 0); return false;
	}

	hr = Device->CreateRenderTargetView(BackBuffer, nullptr, &RenderTargetView);
	BackBuffer->Release();
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateRenderTargetView Failed.", 0, 0);
		return false;
	}
	D3D11_TEXTURE2D_DESC DepthDesc = {};
	DepthDesc.Width = Width;
	DepthDesc.Height = Height;
	DepthDesc.MipLevels = 1;
	DepthDesc.ArraySize = 1;
	DepthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthDesc.SampleDesc.Count = 1;
	DepthDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ID3D11Texture2D* DepthTex = nullptr;
	hr = Device->CreateTexture2D(&DepthDesc, nullptr, &DepthTex);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateTexture2D (Depth) Failed.", 0, 0);
		return false;
	}

	hr = Device->CreateDepthStencilView(DepthTex, nullptr, &DepthStencilView);
	DepthTex->Release();
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateDepthStencilView Failed.", 0, 0); return false;
	}

	Viewport.TopLeftX = 0.f;
	Viewport.TopLeftY = 0.f;
	Viewport.Width = static_cast<float>(Width);
	Viewport.Height = static_cast<float>(Height);
	Viewport.MinDepth = 0.f;
	Viewport.MaxDepth = 1.f;

	if (!CreateConstantBuffer())
	{
		return false;
	}

	if (!ShaderManager.LoadVertexShader(Device, L"..\\Engine\\Shaders\\VertexShader.hlsl"))
	{
		OutputDebugStringW(L"VS Load Failed - 파일 경로 확인\n");
		return false;
	}
	if (!ShaderManager.LoadPixelShader(Device, L"..\\Engine\\Shaders\\PixelShader.hlsl"))
	{
		OutputDebugStringW(L"PS Load Failed - 파일 경로 확인\n");
		return false;
	}


	// Rasterizer State
	D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_BACK;
	hr = Device->CreateRasterizerState(&rsDesc, &RasterizerState);
	if (FAILED(hr))
	{
		MessageBox(0, L"CreateRasterizerState Failed.", 0, 0);
		return false;
	}

	return true;
}

void CRenderer::BeginFrame()
{
	if (GUINewFrame)
	{
		GUINewFrame();
	}

	constexpr float ClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);
	DeviceContext->ClearDepthStencilView(DepthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
	DeviceContext->RSSetViewports(1, &Viewport);
	DeviceContext->RSSetState(RasterizerState);

	CommandList.clear();
}

void CRenderer::EndFrame()
{
	if (GUIUpdate)
	{
		GUIUpdate();
	}

	if (GUIRender)
	{
		GUIRender();
	}

	HRESULT hr = SwapChain->Present(1, 0);
	if (hr == DXGI_STATUS_OCCLUDED)
		bSwapChainOccluded = true;

	if (GUIPostPresent)
	{
		GUIPostPresent();
	}
}

void CRenderer::AddCommand(const FRenderCommand& Command)
{
	CommandList.push_back(Command);
}

void CRenderer::ExecuteCommands()
{
	ShaderManager.Bind(DeviceContext);
	// MeshData 포인터 기준으로 정렬 → 같은 메시끼리 묶어 State Change 최소화
	DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
	std::sort(CommandList.begin(), CommandList.end(),
		[](const FRenderCommand& A, const FRenderCommand& B)
		{
			return A.MeshData < B.MeshData;
		});

	FMeshData* CurrentMesh = nullptr;

	for (const auto& Cmd : CommandList)
	{
		if (!Cmd.MeshData)
		{
			continue;
		}

		// GPU 버퍼가 없으면 생성
		Cmd.MeshData->CreateBuffers(Device);

		// 메시가 바뀔 때만 바인딩
		if (Cmd.MeshData != CurrentMesh)
		{
			Cmd.MeshData->Bind(DeviceContext);
			CurrentMesh = Cmd.MeshData;
		}

		// 오브젝트별 상수 버퍼 업데이트
		UpdateConstantBuffer(Cmd.WorldMatrix, ViewProjectionMatrix);

		// Draw
		DeviceContext->DrawIndexed(Cmd.MeshData->IndexCount, 0, 0);
	}
}

bool CRenderer::CreateConstantBuffer()
{
	D3D11_BUFFER_DESC Desc = {};
	Desc.ByteWidth = sizeof(FConstantBufferData);
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT Hr = Device->CreateBuffer(&Desc, nullptr, &ConstantBuffer);
	if (FAILED(Hr))
	{
		MessageBox(0, L"CreateConstantBuffer Failed.", 0, 0);
		return false;
	}
	return true;
}

void CRenderer::UpdateConstantBuffer(const FMatrix& WorldMatrix, const FMatrix& ViewProj)
{
	FConstantBufferData CBData;
	CBData.WVP = (WorldMatrix * ViewProj).Transpose();
	CBData.World = WorldMatrix.Transpose();

	D3D11_MAPPED_SUBRESOURCE Mapped;
	DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
	memcpy(Mapped.pData, &CBData, sizeof(CBData));
	DeviceContext->Unmap(ConstantBuffer, 0);


}

bool CRenderer::InitOutlineResources()
{
	if (StencilWriteState && StencilTestState && OutlinePS)
		return true;

	// Pass 1: 통상 렌더 + Stencil에 1 쓰기
	D3D11_DEPTH_STENCIL_DESC WriteDesc = {};
	WriteDesc.DepthEnable = TRUE;
	WriteDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	WriteDesc.DepthFunc = D3D11_COMPARISON_LESS;
	WriteDesc.StencilEnable = TRUE;
	WriteDesc.StencilReadMask = 0xFF;
	WriteDesc.StencilWriteMask = 0xFF;
	WriteDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	WriteDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	WriteDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	WriteDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	WriteDesc.BackFace = WriteDesc.FrontFace;

	HRESULT Hr = Device->CreateDepthStencilState(&WriteDesc, &StencilWriteState);
	if (FAILED(Hr)) return false;

	// Pass 2: Stencil이 1이 아닌 곳에만 그리기 (아웃라인)
	D3D11_DEPTH_STENCIL_DESC TestDesc = {};
	TestDesc.DepthEnable = FALSE;
	TestDesc.StencilEnable = TRUE;
	TestDesc.StencilReadMask = 0xFF;
	TestDesc.StencilWriteMask = 0xFF;
	TestDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	TestDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	TestDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	TestDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
	TestDesc.BackFace = TestDesc.FrontFace;

	Hr = Device->CreateDepthStencilState(&TestDesc, &StencilTestState);
	if (FAILED(Hr)) return false;

	// 아웃라인 픽셀 셰이더 로드
	OutlinePS = FShaderMap::Get().GetOrCreatePixelShader(Device, L"..\\Engine\\Shaders\\OutlinePixelShader.hlsl");
	return OutlinePS != nullptr;
}

void CRenderer::RenderOutline(FMeshData* Mesh, const FMatrix& WorldMatrix, float OutlineScale)
{
	if (!Mesh || !InitOutlineResources())
		return;

	Mesh->CreateBuffers(Device);
	Mesh->Bind(DeviceContext);

	// Pass 1: 통상 렌더 + Stencil 마킹 (Ref=1)
	DeviceContext->OMSetDepthStencilState(StencilWriteState, 1);
	UpdateConstantBuffer(WorldMatrix, ViewProjectionMatrix);
	DeviceContext->DrawIndexed(Mesh->IndexCount, 0, 0);

	// Pass 2: 확대된 메시를 아웃라인 셰이더로 그리기 (Stencil != 1인 곳만)
	DeviceContext->OMSetDepthStencilState(StencilTestState, 1);

	// 약간 확대한 WorldMatrix
	FMatrix ScaleUp = FMatrix::Scale(OutlineScale, OutlineScale, OutlineScale);
	FMatrix OutlineWorld = ScaleUp * WorldMatrix;
	UpdateConstantBuffer(OutlineWorld, ViewProjectionMatrix);

	// 아웃라인 셰이더 바인딩
	OutlinePS->Bind(DeviceContext);

	DeviceContext->DrawIndexed(Mesh->IndexCount, 0, 0);

	// 원래 셰이더 복원
	ShaderManager.Bind(DeviceContext);
	// Stencil 상태 복원
	DeviceContext->OMSetDepthStencilState(nullptr, 0);
}

void CRenderer::DrawLine(const FVector& Start, const FVector& End, const FVector4& Color)
{
	FVector Normal = { 0.0f, 0.0f, 0.0f };
	LineVertices.push_back({ Start, Color, Normal });
	LineVertices.push_back({ End, Color, Normal });
}

void CRenderer::ExecuteLineCommands()
{
	if (LineVertices.empty()) return;

	// Depth 테스트 비활성화 (축이 항상 보이도록)
	if (!LineDepthState)
	{
		D3D11_DEPTH_STENCIL_DESC Desc = {};
		Desc.DepthEnable = FALSE;
		Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		Device->CreateDepthStencilState(&Desc, &LineDepthState);
	}
	DeviceContext->OMSetDepthStencilState(LineDepthState, 0);

	// 동적 버퍼 생성/재생성
	UINT BufferSize = static_cast<UINT>(LineVertices.size() * sizeof(FPrimitiveVertex));

	if (LineVertexBuffer)
	{
		LineVertexBuffer->Release();
		LineVertexBuffer = nullptr;
	}

	D3D11_BUFFER_DESC Desc = {};
	Desc.ByteWidth = BufferSize;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = LineVertices.data();

	Device->CreateBuffer(&Desc, &InitData, &LineVertexBuffer);

	// 바인딩
	UINT Stride = sizeof(FPrimitiveVertex);
	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &LineVertexBuffer, &Stride, &Offset);
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// WorldMatrix = Identity로 상수 버퍼 업데이트
	UpdateConstantBuffer(FMatrix::Identity(), ViewProjectionMatrix);

	DeviceContext->Draw(static_cast<UINT>(LineVertices.size()), 0);

	// 토폴로지 복원
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// Depth 상태 복원
	DeviceContext->OMSetDepthStencilState(nullptr, 0);

	LineVertices.clear();
}

void CRenderer::Release()
{
	if (GUIShutdown)
	{
		GUIShutdown();
		GUIInit = nullptr;
		GUIShutdown = nullptr;
		GUINewFrame = nullptr;
		GUIUpdate = nullptr;
		GUIRender = nullptr;
		GUIPostPresent = nullptr;
	}

	if (StencilWriteState)
	{
		StencilWriteState->Release();
		StencilWriteState = nullptr;
	}
	if (StencilTestState)
	{
		StencilTestState->Release();
		StencilTestState = nullptr;
	}
	OutlinePS.reset();
	if (LineVertexBuffer)
	{
		LineVertexBuffer->Release();
		LineVertexBuffer = nullptr;
	}
	if (LineDepthState)
	{
		LineDepthState->Release();
		LineDepthState = nullptr;
	}
	if (RasterizerState)
	{
		RasterizerState->Release();
		RasterizerState = nullptr;
	}
	if (ConstantBuffer)
	{
		ConstantBuffer->Release();
		ConstantBuffer = nullptr;
	}
	if (DepthStencilView)
	{
		DepthStencilView->Release();
		DepthStencilView = nullptr;
	}
	if (RenderTargetView)
	{
		RenderTargetView->Release();
		RenderTargetView = nullptr;
	}
	if (SwapChain)
	{
		SwapChain->Release();
		SwapChain = nullptr;
	}
	if (DeviceContext)
	{
		DeviceContext->Release();
		DeviceContext = nullptr;
	}
	if (Device)
	{
		Device->Release();
		Device = nullptr;
	}
	ShaderManager.Release();
}

bool CRenderer::IsOccluded()
{
	if (bSwapChainOccluded &&
		SwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		return true;

	bSwapChainOccluded = false;
	return false;
}

void CRenderer::OnResize(int NewWidth, int NewHeight)
{
	if (NewWidth == 0 || NewHeight == 0) return;


	DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	RenderTargetView->Release(); RenderTargetView = nullptr;
	DepthStencilView->Release(); DepthStencilView = nullptr;

	SwapChain->ResizeBuffers(0, NewWidth, NewHeight, DXGI_FORMAT_UNKNOWN, 0);


	ID3D11Texture2D* BackBuffer = nullptr;
	SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
	Device->CreateRenderTargetView(BackBuffer, nullptr, &RenderTargetView);
	BackBuffer->Release();

	D3D11_TEXTURE2D_DESC DepthDesc = {};
	DepthDesc.Width = NewWidth;
	DepthDesc.Height = NewHeight;
	DepthDesc.MipLevels = 1;
	DepthDesc.ArraySize = 1;
	DepthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthDesc.SampleDesc.Count = 1;
	DepthDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	ID3D11Texture2D* DepthTex = nullptr;
	Device->CreateTexture2D(&DepthDesc, nullptr, &DepthTex);
	Device->CreateDepthStencilView(DepthTex, nullptr, &DepthStencilView);
	DepthTex->Release();

	// Viewport 갱신
	Viewport.Width = static_cast<float>(NewWidth);
	Viewport.Height = static_cast<float>(NewHeight);
}
