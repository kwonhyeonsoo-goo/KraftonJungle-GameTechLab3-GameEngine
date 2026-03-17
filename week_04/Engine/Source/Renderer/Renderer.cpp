#include "Renderer.h"
#include "ShaderType.h"
#include "Shader.h"
#include "ShaderMap.h"
#include "Material.h"
#include "MaterialManager.h"
#include "Core/Paths.h"
#include "Primitive/PrimitiveBase.h"
#include <cassert>
#include <algorithm>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

uint64 FRenderCommand::MakeSortKey(const FMaterial* InMaterial, const FMeshData* InMeshData)
{
	uint32 MatId = InMaterial ? InMaterial->GetSortId() : 0;
	uint32 MeshId = InMeshData ? InMeshData->GetSortId() : 0;
	return (static_cast<uint64>(MatId) << 32) | MeshId;
}



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

void CRenderer::ClearViewportCallbacks()
{
	if (GUIShutdown)
	{
		GUIShutdown();
	}

	GUIInit = nullptr;
	GUIShutdown = nullptr;
	GUINewFrame = nullptr;
	GUIUpdate = nullptr;
	GUIRender = nullptr;
	GUIPostPresent = nullptr;
	PostRenderCallback = nullptr;
}

bool CRenderer::CreateDeviceAndSwapChain(HWND InHwnd, int32 Width, int32 Height)
{
	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
	SwapChainDesc.BufferDesc.Width = Width;
	SwapChainDesc.BufferDesc.Height = Height;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = 2;
	SwapChainDesc.OutputWindow = InHwnd;
	SwapChainDesc.Windowed = TRUE;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	UINT CreateDeviceFlags = 0;
#ifdef _DEBUG
	CreateDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;
	HRESULT Hr = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		CreateDeviceFlags, &FeatureLevel, 1,
		D3D11_SDK_VERSION, &SwapChainDesc,
		&SwapChain, &Device, nullptr, &DeviceContext
	);

	if (FAILED(Hr))
	{
		MessageBox(nullptr, L"D3D11CreateDeviceAndSwapChain Failed.", nullptr, 0);
		return false;
	}

	return true;
}

bool CRenderer::CreateRenderTargetAndDepthStencil(int32 Width, int32 Height)
{
	// RenderTargetView
	ID3D11Texture2D* BackBuffer = nullptr;
	HRESULT Hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
	if (FAILED(Hr))
	{
		return false;
	}
	Hr = Device->CreateRenderTargetView(BackBuffer, nullptr, &RenderTargetView);
	BackBuffer->Release();
	if (FAILED(Hr))
	{
		return false;
	}

	// DepthStencilView
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
	Hr = Device->CreateTexture2D(&DepthDesc, nullptr, &DepthTex);
	if (FAILED(Hr))
	{
		return false;
	}
	Hr = Device->CreateDepthStencilView(DepthTex, nullptr, &DepthStencilView);
	DepthTex->Release();
	if (FAILED(Hr))
	{
		return false;
	}

	return true;
}

bool CRenderer::Initialize(HWND InHwnd, int32 Width, int32 Height)
{
	Hwnd = InHwnd;

	if (!CreateDeviceAndSwapChain(Hwnd, Width, Height))
	{
		return false;
	}

	if (!CreateRenderTargetAndDepthStencil(Width, Height))
	{
		return false;
	}

	Viewport.TopLeftX = 0.f;
	Viewport.TopLeftY = 0.f;
	Viewport.Width = static_cast<float>(Width);
	Viewport.Height = static_cast<float>(Height);
	Viewport.MinDepth = 0.f;
	Viewport.MaxDepth = 1.f;

	if (!CreateConstantBuffers())
	{
		return false;
	}

	std::wstring ShaderDirW = FPaths::ToWide(FPaths::ShaderDir());
	std::wstring VSPath = ShaderDirW + L"VertexShader.hlsl";
	std::wstring PSPath = ShaderDirW + L"PixelShader.hlsl";

	if (!ShaderManager.LoadVertexShader(Device, VSPath.c_str()))
	{
		return false;
	}
	if (!ShaderManager.LoadPixelShader(Device, PSPath.c_str()))
	{
		return false;
	}

	// 기본 Material 생성 (ColorPixelShader 사용, BaseColor 파라미터 포함)
	{
		auto VS = FShaderMap::Get().GetOrCreateVertexShader(Device, VSPath.c_str());
		std::wstring ColorPSPath = ShaderDirW + L"ColorPixelShader.hlsl";
		auto PS = FShaderMap::Get().GetOrCreatePixelShader(Device, ColorPSPath.c_str());
		DefaultMaterial = std::make_shared<FMaterial>();
		DefaultMaterial->SetOriginName("M_Default");
		DefaultMaterial->SetVertexShader(VS);
		DefaultMaterial->SetPixelShader(PS);

		int32 SlotIndex = DefaultMaterial->CreateConstantBuffer(Device, 16);
		if (SlotIndex >= 0)
		{
			DefaultMaterial->RegisterParameter("BaseColor", SlotIndex, 0, 16);
			float White[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			DefaultMaterial->GetConstantBuffer(SlotIndex)->SetData(White, sizeof(White));
		}

		FMaterialManager::Get().Register("M_Default", DefaultMaterial);
	}

	// Rasterizer State
	D3D11_RASTERIZER_DESC RasterizerDesc = {};
	RasterizerDesc.FillMode = D3D11_FILL_SOLID;
	RasterizerDesc.CullMode = D3D11_CULL_BACK;
	if (FAILED(Device->CreateRasterizerState(&RasterizerDesc, &RasterizerState)))
	{
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

	PrevCommandCount = CommandList.size();
	CommandList.clear();
	CommandList.reserve(PrevCommandCount);
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

	UINT SyncInterval = bVSyncEnabled ? 1 : 0;
	HRESULT Hr = SwapChain->Present(SyncInterval, 0);
	if (Hr == DXGI_STATUS_OCCLUDED)
		bSwapChainOccluded = true;

	if (GUIPostPresent)
	{
		GUIPostPresent();
	}
}

void CRenderer::SubmitCommands(const FRenderCommandQueue& Queue)
{
	// 큐의 카메라 데이터를 적용
	ViewMatrix = Queue.ViewMatrix;
	ProjectionMatrix = Queue.ProjectionMatrix;

	// GPU 버퍼 보장 + 내부 CommandList로 이전
	for (const auto& Cmd : Queue.Commands)
	{
		if (Cmd.MeshData)
		{
			Cmd.MeshData->CreateBuffers(Device);
		}
		AddCommand(Cmd);
	}
}

void CRenderer::AddCommand(const FRenderCommand& Command)
{
	CommandList.push_back(Command);
	FRenderCommand& Added = CommandList.back();

	// Material 미지정 시 DefaultMaterial 할당
	if (!Added.Material)
	{
		Added.Material = DefaultMaterial.get();
	}
	Added.SortKey = FRenderCommand::MakeSortKey(Added.Material, Added.MeshData);
}

void CRenderer::ExecuteCommands()
{
	// 프레임 상수 버퍼 업데이트 및 바인딩 (b0: Frame, b1: Object)
	UpdateFrameConstantBuffer();
	ID3D11Buffer* CBs[2] = { FrameConstantBuffer, ObjectConstantBuffer };
	DeviceContext->VSSetConstantBuffers(0, 2, CBs);

	// SortKey 기준 정렬 → Material, MeshData 순 State Change 최소화
	std::sort(CommandList.begin(), CommandList.end(),
		[](const FRenderCommand& A, const FRenderCommand& B)
		{
			return A.SortKey < B.SortKey;
		});

	FMaterial* CurrentMaterial = nullptr;
	FMeshData* CurrentMesh = nullptr;

	for (const auto& Cmd : CommandList)
	{
		if (!Cmd.MeshData)
		{
			continue;
		}

		// Material이 바뀔 때만 셰이더 바인딩
		if (Cmd.Material != CurrentMaterial)
		{
			Cmd.Material->Bind(DeviceContext);
			CurrentMaterial = Cmd.Material;
		}

		// 메시가 바뀔 때만 바인딩
		if (Cmd.MeshData != CurrentMesh)
		{
			Cmd.MeshData->Bind(DeviceContext);
			CurrentMesh = Cmd.MeshData;
		}

		// 오브젝트별 상수 버퍼 업데이트
		UpdateObjectConstantBuffer(Cmd.WorldMatrix);

		// Draw
		DeviceContext->DrawIndexed(Cmd.MeshData->IndexCount, 0, 0);
	}

	// 메시 커맨드 실행 후 PostRender (아웃라인, 라인 등)
	if (PostRenderCallback)
	{
		PostRenderCallback(this);
	}
}

bool CRenderer::CreateConstantBuffers()
{
	D3D11_BUFFER_DESC Desc = {};
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	// b0: Frame (View + Projection)
	Desc.ByteWidth = sizeof(FFrameConstantBuffer);
	HRESULT Hr = Device->CreateBuffer(&Desc, nullptr, &FrameConstantBuffer);
	if (FAILED(Hr))
	{
		MessageBox(0, L"CreateConstantBuffer (Frame) Failed.", 0, 0);
		return false;
	}

	// b1: Object (World)
	Desc.ByteWidth = sizeof(FObjectConstantBuffer);
	Hr = Device->CreateBuffer(&Desc, nullptr, &ObjectConstantBuffer);
	if (FAILED(Hr))
	{
		MessageBox(0, L"CreateConstantBuffer (Object) Failed.", 0, 0);
		return false;
	}

	return true;
}

void CRenderer::UpdateFrameConstantBuffer()
{
	FFrameConstantBuffer CBData;
	CBData.View = ViewMatrix.GetTransposed();
	CBData.Projection = ProjectionMatrix.GetTransposed();

	D3D11_MAPPED_SUBRESOURCE Mapped;
	DeviceContext->Map(FrameConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
	memcpy(Mapped.pData, &CBData, sizeof(CBData));
	DeviceContext->Unmap(FrameConstantBuffer, 0);
}

void CRenderer::UpdateObjectConstantBuffer(const FMatrix& WorldMatrix)
{
	FObjectConstantBuffer CBData;
	CBData.World = WorldMatrix.GetTransposed();

	D3D11_MAPPED_SUBRESOURCE Mapped;
	DeviceContext->Map(ObjectConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
	memcpy(Mapped.pData, &CBData, sizeof(CBData));
	DeviceContext->Unmap(ObjectConstantBuffer, 0);
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
	WriteDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;
	WriteDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
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
	std::wstring OutlinePSPath = FPaths::ToWide(FPaths::ShaderDir() + "OutlinePixelShader.hlsl");
	OutlinePS = FShaderMap::Get().GetOrCreatePixelShader(Device, OutlinePSPath.c_str());
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
	UpdateObjectConstantBuffer(WorldMatrix);
	DeviceContext->DrawIndexed(Mesh->IndexCount, 0, 0);

	// Pass 2: 확대된 메시를 아웃라인 셰이더로 그리기 (Stencil != 1인 곳만)
	DeviceContext->OMSetDepthStencilState(StencilTestState, 1);

	// 약간 확대한 WorldMatrix
	FMatrix ScaleUp = FMatrix::MakeScale(OutlineScale);
	FMatrix OutlineWorld = ScaleUp * WorldMatrix;
	UpdateObjectConstantBuffer(OutlineWorld);

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

	// 기본 셰이더 복원 (ExecuteCommands 후 마지막 Material 셰이더가 남아있을 수 있음)
	ShaderManager.Bind(DeviceContext);
	ID3D11Buffer* CBs[2] = { FrameConstantBuffer, ObjectConstantBuffer };
	DeviceContext->VSSetConstantBuffers(0, 2, CBs);

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
	UpdateObjectConstantBuffer(FMatrix::Identity);

	DeviceContext->Draw(static_cast<UINT>(LineVertices.size()), 0);

	// 토폴로지 복원
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// Depth 상태 복원
	DeviceContext->OMSetDepthStencilState(nullptr, 0);

	LineVertices.clear();
}

void CRenderer::Release()
{
	ClearViewportCallbacks();
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
	DefaultMaterial.reset();
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
	if (FrameConstantBuffer)
	{
		FrameConstantBuffer->Release();
		FrameConstantBuffer = nullptr;
	}
	if (ObjectConstantBuffer)
	{
		ObjectConstantBuffer->Release();
		ObjectConstantBuffer = nullptr;
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

void CRenderer::OnResize(int32 NewWidth, int32 NewHeight)
{
	if (NewWidth == 0 || NewHeight == 0) return;

	DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

	if (RenderTargetView) { RenderTargetView->Release(); RenderTargetView = nullptr; }
	if (DepthStencilView) { DepthStencilView->Release(); DepthStencilView = nullptr; }

	SwapChain->ResizeBuffers(0, NewWidth, NewHeight, DXGI_FORMAT_UNKNOWN, 0);

	CreateRenderTargetAndDepthStencil(NewWidth, NewHeight);

	Viewport.Width = static_cast<float>(NewWidth);
	Viewport.Height = static_cast<float>(NewHeight);
}
