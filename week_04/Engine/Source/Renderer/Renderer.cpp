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

#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb_image.h"

static FVector GetCameraWorldPositionFromViewMatrix(const FMatrix& ViewMatrix)
{
	const FMatrix InvView = ViewMatrix.GetInverse();
	return FVector(InvView.M[3][0], InvView.M[3][1], InvView.M[3][2]);
}
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

void CRenderer::SetSceneRenderTarget(ID3D11RenderTargetView* InRenderTargetView, ID3D11DepthStencilView* InDepthStencilView, const D3D11_VIEWPORT& InViewport)
{
	SceneRenderTargetView = InRenderTargetView;
	SceneDepthStencilView = InDepthStencilView;
	SceneViewport = InViewport;
	bUseSceneRenderTargetOverride = (SceneRenderTargetView != nullptr && SceneDepthStencilView != nullptr);
}

void CRenderer::ClearSceneRenderTarget()
{
	SceneRenderTargetView = nullptr;
	SceneDepthStencilView = nullptr;
	SceneViewport = {};
	bUseSceneRenderTargetOverride = false;
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

	std::wstring ShaderDirW = FPaths::ShaderDir();
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

	D3D11_RASTERIZER_DESC NoCullDesc = RasterizerDesc;
	NoCullDesc.CullMode = D3D11_CULL_NONE;
	HRESULT Hr = Device->CreateRasterizerState(&NoCullDesc, &NoCullRasterizerState);
	if (FAILED(Hr))
	{
		MessageBox(0, L"CreateRasterizerState (NoCull) Failed.", 0, 0);
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC OverlayDepthDesc = {};
	OverlayDepthDesc.DepthEnable = FALSE;
	OverlayDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	OverlayDepthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	Hr = Device->CreateDepthStencilState(&OverlayDepthDesc, &OverlayDepthState);
	if (FAILED(Hr))
	{
		MessageBox(0, L"CreateDepthStencilState (Overlay) Failed.", 0, 0);
		return false;
	}

	if (!TextRenderer.Initialize(Device, DeviceContext))
	{
		MessageBox(0, L"TextRenderer Initialize Failed.", 0, 0);
		return false;
	}

	std::filesystem::path FolderIconPath = FPaths::AssetDir() / FString("\\Textures\\FolderIcon.png");
	std::filesystem::path FileIconPath = FPaths::AssetDir() / FString("\\Textures\\FileIcon.png");

	FString FolderIconPathString = FolderIconPath.string();
	FString FileIconPathString = FileIconPath.string();

	CreateTextureFromSTB(Device, FolderIconPathString.c_str(), &FolderIconSRV);
	CreateTextureFromSTB(Device, FileIconPathString.c_str(), &FileIconSRV);

	return true;
}

void CRenderer::BeginFrame()
{
	if (GUINewFrame)
	{
		GUINewFrame();
	}

	if (GUIUpdate)
	{
		GUIUpdate();
	}

	constexpr float ClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	if (RenderTargetView)
	{
		DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);
	}
	if (DepthStencilView)
	{
		DeviceContext->ClearDepthStencilView(DepthStencilView,
			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	ID3D11RenderTargetView* ActiveRenderTargetView = RenderTargetView;
	ID3D11DepthStencilView* ActiveDepthStencilView = DepthStencilView;
	D3D11_VIEWPORT ActiveViewport = Viewport;

	if (bUseSceneRenderTargetOverride)
	{
		ActiveRenderTargetView = SceneRenderTargetView;
		ActiveDepthStencilView = SceneDepthStencilView;
		ActiveViewport = SceneViewport;

		if (ActiveRenderTargetView && ActiveRenderTargetView != RenderTargetView)
		{
			DeviceContext->ClearRenderTargetView(ActiveRenderTargetView, ClearColor);
		}
		if (ActiveDepthStencilView && ActiveDepthStencilView != DepthStencilView)
		{
			DeviceContext->ClearDepthStencilView(ActiveDepthStencilView,
				D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		}
	}

	DeviceContext->OMSetRenderTargets(1, &ActiveRenderTargetView, ActiveDepthStencilView);
	DeviceContext->RSSetViewports(1, &ActiveViewport);
	DeviceContext->RSSetState(RasterizerState);

	PrevCommandCount = CommandList.size();
	CommandList.clear();
	CommandList.reserve(PrevCommandCount);

	TextCommandList.clear();
	TextCommandList.reserve(PrevCommandCount);
}

void CRenderer::EndFrame()
{
	if (RenderTargetView)
	{
		DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
		DeviceContext->RSSetViewports(1, &Viewport);
		DeviceContext->RSSetState(RasterizerState);
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
	for (const auto& TextCmd : Queue.TextCommands)
	{
		TextCommandList.push_back(TextCmd);
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

	std::sort(CommandList.begin(), CommandList.end(),
		[](const FRenderCommand& A, const FRenderCommand& B)
		{
			return A.SortKey < B.SortKey;
		});

	auto ExecutePass = [this, &CBs](bool bOverlayPass)
		{
			FMaterial* CurrentMaterial = nullptr;
			FMeshData* CurrentMesh = nullptr;
			D3D11_PRIMITIVE_TOPOLOGY CurrentMeshTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
			ID3D11RasterizerState* CurrentRasterizerState = nullptr;
			ID3D11DepthStencilState* CurrentDepthStencilState = nullptr;

			for (const auto& Cmd : CommandList)
			{
				if (Cmd.bOverlay != bOverlayPass || !Cmd.MeshData || Cmd.MeshData->Indices.empty())
				{
					continue;
				}

				if (Cmd.Material != CurrentMaterial)
				{
					Cmd.Material->Bind(DeviceContext);
					CurrentMaterial = Cmd.Material;
					DeviceContext->VSSetConstantBuffers(0, 2, CBs);
				}

				if (Cmd.MeshData != CurrentMesh)
				{
					Cmd.MeshData->Bind(DeviceContext);
					CurrentMesh = Cmd.MeshData;
				}

				D3D11_PRIMITIVE_TOPOLOGY DesiredMeshTopology = (D3D11_PRIMITIVE_TOPOLOGY)CurrentMesh->Topology;
				if (DesiredMeshTopology != CurrentMeshTopology)
				{
					DeviceContext->IASetPrimitiveTopology(DesiredMeshTopology);
				}

				ID3D11RasterizerState* DesiredRasterizerState = Cmd.bDisableCulling ? NoCullRasterizerState : RasterizerState;
				if (DesiredRasterizerState != CurrentRasterizerState)
				{
					DeviceContext->RSSetState(DesiredRasterizerState);
					CurrentRasterizerState = DesiredRasterizerState;
				}

				ID3D11DepthStencilState* DesiredDepthStencilState = (Cmd.bDisableDepthTest || Cmd.bDisableDepthWrite) ? OverlayDepthState : nullptr;
				if (DesiredDepthStencilState != CurrentDepthStencilState)
				{
					DeviceContext->OMSetDepthStencilState(DesiredDepthStencilState, 0);
					CurrentDepthStencilState = DesiredDepthStencilState;
				}

				UpdateObjectConstantBuffer(Cmd.WorldMatrix);
				DeviceContext->DrawIndexed(Cmd.MeshData->Indices.size(), 0, 0);
			}
		};

	ExecutePass(false);
	ExecutePass(true);

	DeviceContext->OMSetDepthStencilState(nullptr, 0);
	DeviceContext->RSSetState(RasterizerState);
	ShaderManager.Bind(DeviceContext);
	DeviceContext->VSSetConstantBuffers(0, 2, CBs);

	const FVector CameraPosition = GetCameraWorldPositionFromViewMatrix(ViewMatrix);

	if (!TextCommandList.empty() || bEnableTextRenderTest)
	{
		TextRenderer.Begin(ViewMatrix, ProjectionMatrix, CameraPosition);

		for (const FTextRenderCommand& TextCmd : TextCommandList)
		{
			TextRenderer.DrawTextBillboard(
				TextCmd.Text,
				TextCmd.WorldPosition,
				TextCmd.WorldScale,
				TextCmd.Color
			);
		}

		// 테스트용 코드
		if (bEnableTextRenderTest)
		{
			TextRenderer.Begin(ViewMatrix, ProjectionMatrix, CameraPosition);

			TextRenderer.DrawTextBillboard(
				FString("012TEST"),
				FVector(0.0f, 0.0f, 0.0f),
				0.3f,
				FVector4(1.0f, 1.0f, 1.0f, 1.0f)
			);
		}
	}

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

bool CRenderer::CreateTextureFromSTB(
	ID3D11Device* Device,
	const char* FilePath,
	ID3D11ShaderResourceView** OutSRV)
{
	int Width, Height, Channels;
	unsigned char* Data = stbi_load(FilePath, &Width, &Height, &Channels, 4); // RGBA

	if (!Data)
		return false;

	// Texture 생성
	D3D11_TEXTURE2D_DESC Desc = {};
	Desc.Width = Width;
	Desc.Height = Height;
	Desc.MipLevels = 1;
	Desc.ArraySize = 1;
	Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	Desc.SampleDesc.Count = 1;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = Data;
	InitData.SysMemPitch = Width * 4;

	ID3D11Texture2D* Texture = nullptr;
	HRESULT hr = Device->CreateTexture2D(&Desc, &InitData, &Texture);

	stbi_image_free(Data);

	if (FAILED(hr))
		return false;

	// SRV 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = Desc.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	hr = Device->CreateShaderResourceView(Texture, &SRVDesc, OutSRV);

	Texture->Release();

	return SUCCEEDED(hr);
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
	FString OutlinePSPathString = (FPaths::ShaderDir() / "OutlinePixelShader.hlsl").string();
	std::wstring OutlinePSPath = std::wstring(OutlinePSPathString.begin(), OutlinePSPathString.end());	

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
	DeviceContext->DrawIndexed(Mesh->Indices.size(), 0, 0);

	// Pass 2: 확대된 메시를 아웃라인 셰이더로 그리기 (Stencil != 1인 곳만)
	DeviceContext->OMSetDepthStencilState(StencilTestState, 1);

	// 약간 확대한 WorldMatrix
	FMatrix ScaleUp = FMatrix::MakeScale(OutlineScale);
	FMatrix OutlineWorld = ScaleUp * WorldMatrix;
	UpdateObjectConstantBuffer(OutlineWorld);

	// 아웃라인 셰이더 바인딩
	OutlinePS->Bind(DeviceContext);

	DeviceContext->DrawIndexed(Mesh->Indices.size(), 0, 0);

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

void CRenderer::DrawCube(const FVector& Center, const FVector& BoxExtent, const FVector4& Color)
{
	// 8개 꼭짓점 생성
	FVector v000 = Center + FVector(-BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z);
	FVector v001 = Center + FVector(-BoxExtent.X, -BoxExtent.Y, BoxExtent.Z);
	FVector v010 = Center + FVector(-BoxExtent.X, BoxExtent.Y, -BoxExtent.Z);
	FVector v011 = Center + FVector(-BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
	FVector v100 = Center + FVector(BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z);
	FVector v101 = Center + FVector(BoxExtent.X, -BoxExtent.Y, BoxExtent.Z);
	FVector v110 = Center + FVector(BoxExtent.X, BoxExtent.Y, -BoxExtent.Z);
	FVector v111 = Center + FVector(BoxExtent.X, BoxExtent.Y, BoxExtent.Z);

	// --- 아래 사각형 (Z-)
	DrawLine(v000, v100, Color);
	DrawLine(v100, v110, Color);
	DrawLine(v110, v010, Color);
	DrawLine(v010, v000, Color);

	// --- 위 사각형 (Z+)
	DrawLine(v001, v101, Color);
	DrawLine(v101, v111, Color);
	DrawLine(v111, v011, Color);
	DrawLine(v011, v001, Color);

	// --- 수직 연결
	DrawLine(v000, v001, Color);
	DrawLine(v100, v101, Color);
	DrawLine(v110, v111, Color);
	DrawLine(v010, v011, Color);
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

	// 동적 버퍼 재사용, 불가능하면 새로 생성.
	UINT BufferSize = static_cast<UINT>(LineVertices.size() * sizeof(FPrimitiveVertex));

	if (LineVertexBuffer && LineVertexBufferSize < BufferSize)
	{
		LineVertexBuffer->Release();
		LineVertexBuffer = nullptr;
		LineVertexBufferSize = 0;
	}

	if (!LineVertexBuffer)
	{
		D3D11_BUFFER_DESC Desc = {};
		Desc.ByteWidth = BufferSize;
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		Device->CreateBuffer(&Desc, nullptr, &LineVertexBuffer);
		LineVertexBufferSize = BufferSize;
	}

	// 버퍼에 메모리 카피
	D3D11_MAPPED_SUBRESOURCE Mapped;
	if (SUCCEEDED(DeviceContext->Map(LineVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
	{
		memcpy(Mapped.pData, LineVertices.data(), BufferSize);
		DeviceContext->Unmap(LineVertexBuffer, 0);
	}

	// 바인딩
	UINT Stride = sizeof(FPrimitiveVertex);
	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &LineVertexBuffer, &Stride, &Offset);
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// WorldMatrix = Identity로 상수 버퍼 업데이트
	// => 월드 좌표 (0,0,0) 기준으로 그리기
	UpdateObjectConstantBuffer(FMatrix::Identity);

	DeviceContext->Draw(static_cast<UINT>(LineVertices.size()), 0);

	// Depth 상태 복원
	DeviceContext->OMSetDepthStencilState(nullptr, 0);

	LineVertices.clear();
}

void CRenderer::Release()
{
	ClearViewportCallbacks();
	ClearSceneRenderTarget();

	TextRenderer.Release();

	ShaderManager.Release();
	FShaderMap::Get().Clear();
	FMaterialManager::Get().Clear();
	
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
	if (OverlayDepthState)
	{
		OverlayDepthState->Release();
		OverlayDepthState = nullptr;
	}

	if (RasterizerState)
	{
		RasterizerState->Release();
		RasterizerState = nullptr;
	}
	if (NoCullRasterizerState)
	{
		NoCullRasterizerState->Release();
		NoCullRasterizerState = nullptr;
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

	ClearSceneRenderTarget();

	DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

	if (RenderTargetView) { RenderTargetView->Release(); RenderTargetView = nullptr; }
	if (DepthStencilView) { DepthStencilView->Release(); DepthStencilView = nullptr; }

	SwapChain->ResizeBuffers(0, NewWidth, NewHeight, DXGI_FORMAT_UNKNOWN, 0);

	CreateRenderTargetAndDepthStencil(NewWidth, NewHeight);

	Viewport.Width = static_cast<float>(NewWidth);
	Viewport.Height = static_cast<float>(NewHeight);
}
