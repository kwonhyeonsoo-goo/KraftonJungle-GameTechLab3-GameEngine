#include "Renderer.h"
#include "ShaderType.h"
#include "Shader.h"
#include "ShaderMap.h"
#include "Material.h"
#include "MaterialManager.h"
#include "Core/FEngine.h"
#include "Core/Paths.h"
#include "Primitive/PrimitiveBase.h"
#include <cassert>
#include <algorithm>
#include <cctype>
#include "Debug/EngineLog.h"
#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb_image.h"
#include <Asset/AssetManager.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

ENGINE_API FRenderer* GRenderer = nullptr;

static FVector GetCameraWorldPositionFromViewMatrix(const FMatrix& ViewMatrix)
{
	const FMatrix InvView = ViewMatrix.GetInverse();
	return FVector(InvView.M[3][0], InvView.M[3][1], InvView.M[3][2]);
}

FRenderer::FRenderer(HWND InHwnd, int32 InWidth, int32 InHeight)
{
	Initialize(InHwnd, InWidth, InHeight);
}

FRenderer::~FRenderer()
{
	Release();
}

void FRenderer::SetLevelRenderTarget(ID3D11RenderTargetView* InRenderTargetView, ID3D11DepthStencilView* InDepthStencilView, const D3D11_VIEWPORT& InViewport)
{
	LevelRenderTargetView = InRenderTargetView;
	LevelDepthStencilView = InDepthStencilView;
	LevelViewport = InViewport;
	bUseLevelRenderTargetOverride = (LevelRenderTargetView != nullptr && LevelDepthStencilView != nullptr);
}

void FRenderer::ClearLevelRenderTarget()
{
	LevelRenderTargetView = nullptr;
	LevelDepthStencilView = nullptr;
	LevelViewport = {};
	bUseLevelRenderTargetOverride = false;
}

void FRenderer::SetGUICallbacks(
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

void FRenderer::SetGUIUpdateCallback(FGUICallback InUpdate)
{
	GUIUpdate = std::move(InUpdate);
}

void FRenderer::ClearViewportCallbacks()
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

bool FRenderer::CreateDeviceAndSwapChain(HWND InHwnd, int32 Width, int32 Height)
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

bool FRenderer::CreateRenderTargetAndDepthStencil(int32 Width, int32 Height)
{
	ID3D11Texture2D* BackBuffer = nullptr;
	HRESULT Hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
	if (FAILED(Hr)) return false;
	
	Hr = Device->CreateRenderTargetView(BackBuffer, nullptr, &RenderTargetView);
	BackBuffer->Release();
	if (FAILED(Hr)) return false;

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
	if (FAILED(Hr)) return false;
	
	Hr = Device->CreateDepthStencilView(DepthTex, nullptr, &DepthStencilView);
	DepthTex->Release();
	D3D11_TEXTURE2D_DESC PickDesc = {};
	PickDesc.Width = Width;
	PickDesc.Height = Height;
	PickDesc.MipLevels = 1;
	PickDesc.ArraySize = 1;
	PickDesc.Format = DXGI_FORMAT_R32_UINT;
	PickDesc.SampleDesc.Count = 1;
	PickDesc.Usage = D3D11_USAGE_DEFAULT;
	PickDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
	if (FAILED(Device->CreateTexture2D(&PickDesc, nullptr, &PickingTexture))) return false;
	if (FAILED(Device->CreateRenderTargetView(PickingTexture, nullptr, &PickingRTV))) return false;

	// --- 피킹용 Staging 텍스처 (CPU 읽기용) ---
	PickDesc.Usage = D3D11_USAGE_STAGING;
	PickDesc.BindFlags = 0;
	PickDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	if (FAILED(Device->CreateTexture2D(&PickDesc, nullptr, &PickingStagingTexture))) return false;

	// --- 피킹용 전용 깊이 버퍼 ---
	D3D11_TEXTURE2D_DESC PickDepthDesc = DepthDesc; // 기존 DepthDesc 재사용
	ID3D11Texture2D* PickDepthTex = nullptr;
	if (FAILED(Device->CreateTexture2D(&PickDepthDesc, nullptr, &PickDepthTex))) return false;
	if (FAILED(Device->CreateDepthStencilView(PickDepthTex, nullptr, &PickingDSV))) return false;
	PickDepthTex->Release();
	return SUCCEEDED(Hr);
}

bool FRenderer::Initialize(HWND InHwnd, int32 Width, int32 Height)
{
	Hwnd = InHwnd;

	if (!CreateDeviceAndSwapChain(Hwnd, Width, Height)) return false;
	if (!CreateRenderTargetAndDepthStencil(Width, Height)) return false;

	Viewport.TopLeftX = 0.f;
	Viewport.TopLeftY = 0.f;
	Viewport.Width = static_cast<float>(Width);
	Viewport.Height = static_cast<float>(Height);
	Viewport.MinDepth = 0.f;
	Viewport.MaxDepth = 1.f;

	RenderStateManager = std::make_unique<FRenderStateManager>(Device, DeviceContext);
	RenderStateManager->PrepareCommonStates();
	FMaterialManager::Get().CreateDefaultWhiteTexture(Device);
	if (!CreateConstantBuffers()) return false;
	SetConstantBuffers();
	{
		D3D11_SAMPLER_DESC SamplerDesc = {};
		SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		SamplerDesc.MinLOD = 0;
		SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		if (FAILED(Device->CreateSamplerState(&SamplerDesc, &NormalSampler))) return false;
	}


	std::wstring ShaderDirW = FPaths::ShaderDir();
	std::wstring VSPath = ShaderDirW + L"VertexShader.hlsl";
	std::wstring PSPath = ShaderDirW + L"PixelShader.hlsl";

	std::wstring InstancedVSPath = ShaderDirW + L"InstancedVertexShader.hlsl";

	D3D11_INPUT_ELEMENT_DESC InstancedLayout[] =
	{
		// slot 0: per-vertex
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,        0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		// slot 1: per-instance
		{ "WORLD",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  1, 0,                            D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "WORLD",    1, DXGI_FORMAT_R32G32B32A32_FLOAT,  1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "WORLD",    2, DXGI_FORMAT_R32G32B32A32_FLOAT,  1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "WORLD",    3, DXGI_FORMAT_R32G32B32A32_FLOAT,  1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32_UINT,         1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	InstancedVertexShader = FShaderMap::Get().GetOrCreateVertexShaderWithLayout(
		Device,
		InstancedVSPath.c_str(),
		InstancedLayout,
		ARRAYSIZE(InstancedLayout)
	);
	if (!InstancedVertexShader)
	{
		MessageBox(0, L"Instanced Vertex Shader Load Failed.", 0, 0);
		return false;
	}
	if (!ShaderManager.LoadVertexShader(Device, VSPath.c_str())) return false;
	if (!ShaderManager.LoadPixelShader(Device, PSPath.c_str())) return false;

	/** 기본 Material 생성 */
	{
		auto VS = FShaderMap::Get().GetOrCreateVertexShader(Device, VSPath.c_str());
		std::wstring ColorPSPath = ShaderDirW + L"ColorPixelShader.hlsl";
		auto PS = FShaderMap::Get().GetOrCreatePixelShader(Device, ColorPSPath.c_str());
		DefaultMaterial = std::make_shared<FMaterial>();
		DefaultMaterial->SetOriginName("M_Default");
		DefaultMaterial->SetVertexShader(VS);
		DefaultMaterial->SetPixelShader(PS);

		FRasterizerStateOption rasterizerOption;
		rasterizerOption.FillMode = D3D11_FILL_SOLID;
		rasterizerOption.CullMode = D3D11_CULL_BACK;
		auto RS = RenderStateManager->GetOrCreateRasterizerState(rasterizerOption);
		DefaultMaterial->SetRasterizerOption(rasterizerOption);
		DefaultMaterial->SetRasterizerState(RS);

		FDepthStencilStateOption depthStencilOption;
		depthStencilOption.DepthEnable = true;
		depthStencilOption.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		auto DSS = RenderStateManager->GetOrCreateDepthStencilState(depthStencilOption);
		DefaultMaterial->SetDepthStencilOption(depthStencilOption);
		DefaultMaterial->SetDepthStencilState(DSS);

		auto PickingPS = FShaderMap::Get().GetOrCreatePixelShader(Device, (ShaderDirW + L"PickingPixelShader.hlsl").c_str());
		PickingMaterial = std::make_shared<FMaterial>();
		PickingMaterial->SetOriginName("M_Picking");
		PickingMaterial->SetVertexShader(InstancedVertexShader);
		PickingMaterial->SetPixelShader(PickingPS);
		FRasterizerStateOption PickRasterOption;
		PickRasterOption.FillMode = D3D11_FILL_SOLID;
		PickRasterOption.CullMode = D3D11_CULL_BACK;
		PickingMaterial->SetRasterizerOption(PickRasterOption);
		PickingMaterial->SetRasterizerState(RenderStateManager->GetOrCreateRasterizerState(PickingMaterial->GetRasterizerOption()));
		FDepthStencilStateOption PickDepthOption;
		PickDepthOption.DepthEnable = true;
		PickDepthOption.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		PickingMaterial->SetDepthStencilOption(PickDepthOption);
		PickingMaterial->SetDepthStencilState(RenderStateManager->GetOrCreateDepthStencilState(PickDepthOption));
		FBlendStateOption PickBlendOption;
		PickBlendOption.BlendEnable = false;
		PickingMaterial->SetBlendOption(PickBlendOption);
		PickingMaterial->SetBlendState(RenderStateManager->GetOrCreateBlendState(PickBlendOption));
		int32 SlotIndex = DefaultMaterial->CreateConstantBuffer(Device, 16);
		if (SlotIndex >= 0)
		{
			DefaultMaterial->RegisterParameter("BaseColor", SlotIndex, 0, 16);
			float White[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			DefaultMaterial->GetConstantBuffer(SlotIndex)->SetData(White, sizeof(White));
		}
		FMaterialManager::Get().Register("M_Default", DefaultMaterial);
	}

	/** Texture 용 Material 생성 */
	{
		auto VS = FShaderMap::Get().GetOrCreateVertexShader(Device, VSPath.c_str());
		std::wstring TexturePSPath = ShaderDirW + L"TexturePixelShader.hlsl";
		auto PS = FShaderMap::Get().GetOrCreatePixelShader(Device, TexturePSPath.c_str());
		DefaultTextureMaterial = std::make_shared<FMaterial>();
		DefaultTextureMaterial->SetOriginName("M_Default");
		DefaultTextureMaterial->SetVertexShader(VS);
		DefaultTextureMaterial->SetPixelShader(PS);

		FRasterizerStateOption rasterizerOption;
		rasterizerOption.FillMode = D3D11_FILL_SOLID;
		rasterizerOption.CullMode = D3D11_CULL_BACK;
		auto RS = RenderStateManager->GetOrCreateRasterizerState(rasterizerOption);
		DefaultTextureMaterial->SetRasterizerOption(rasterizerOption);
		DefaultTextureMaterial->SetRasterizerState(RS);

		FDepthStencilStateOption depthStencilOption;
		depthStencilOption.DepthEnable = true;
		depthStencilOption.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		auto DSS = RenderStateManager->GetOrCreateDepthStencilState(depthStencilOption);
		DefaultTextureMaterial->SetDepthStencilOption(depthStencilOption);
		DefaultTextureMaterial->SetDepthStencilState(DSS);

		int32 SlotIndex = DefaultTextureMaterial->CreateConstantBuffer(Device, 16);
		if (SlotIndex >= 0)
		{
			DefaultTextureMaterial->RegisterParameter("BaseColor", SlotIndex, 0, 16);
			float White[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			DefaultTextureMaterial->GetConstantBuffer(SlotIndex)->SetData(White, sizeof(White));
		}
		FMaterialManager::Get().Register("M_Default_Texture", DefaultTextureMaterial);
	}

	/** Overlay 색상용 Material 생성 */
	{
		auto VS = FShaderMap::Get().GetOrCreateVertexShader(Device, VSPath.c_str());
		std::wstring OverlayColorPSPath = ShaderDirW + L"OverlayColorPixelShader.hlsl";
		auto PS = FShaderMap::Get().GetOrCreatePixelShader(Device, OverlayColorPSPath.c_str());
		OverlayColorMaterial = std::make_shared<FMaterial>();
		OverlayColorMaterial->SetOriginName("M_OverlayColor");
		OverlayColorMaterial->SetVertexShader(VS);
		OverlayColorMaterial->SetPixelShader(PS);

		FRasterizerStateOption RasterizerOption;
		RasterizerOption.FillMode = D3D11_FILL_SOLID;
		RasterizerOption.CullMode = D3D11_CULL_NONE;
		OverlayColorMaterial->SetRasterizerOption(RasterizerOption);
		OverlayColorMaterial->SetRasterizerState(RenderStateManager->GetOrCreateRasterizerState(RasterizerOption));

		FDepthStencilStateOption DepthOption;
		DepthOption.DepthEnable = false;
		DepthOption.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		OverlayColorMaterial->SetDepthStencilOption(DepthOption);
		OverlayColorMaterial->SetDepthStencilState(RenderStateManager->GetOrCreateDepthStencilState(DepthOption));

		FBlendStateOption BlendOption;
		BlendOption.BlendEnable = true;
		BlendOption.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		BlendOption.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		BlendOption.BlendOp = D3D11_BLEND_OP_ADD;
		BlendOption.SrcBlendAlpha = D3D11_BLEND_ONE;
		BlendOption.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		BlendOption.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		OverlayColorMaterial->SetBlendOption(BlendOption);
		OverlayColorMaterial->SetBlendState(RenderStateManager->GetOrCreateBlendState(BlendOption));

		int32 SlotIndex = OverlayColorMaterial->CreateConstantBuffer(Device, 16);
		if (SlotIndex >= 0)
		{
			OverlayColorMaterial->RegisterParameter("BaseColor", SlotIndex, 0, 16);
			FVector4 White(1.0f, 1.0f, 1.0f, 1.0f);
			OverlayColorMaterial->SetParameterData("BaseColor", &White, 16);
		}
	}

	if (!TextRenderer.Initialize(this)) return false;

	FString SubUVPathStr = FPaths::ToAbsolutePath("Content/Textures/SubUVDino.png");
	if (!SubUVRenderer.Initialize(this, FPaths::ToWide(SubUVPathStr)))
	{
		MessageBox(0, L"SubUVRenderer Initialize Failed.", 0, 0);
	}

	FString FolderIconPathStr = FPaths::ToAbsolutePath("Assets/Textures/FolderIcon.png");
	FString FileIconPathStr = FPaths::ToAbsolutePath("Assets/Textures/FileIcon.png");

	CreateTextureFromSTB(Device, FPaths::ToWide(FolderIconPathStr).c_str(), &FolderIconSRV);
	CreateTextureFromSTB(Device, FPaths::ToWide(FileIconPathStr).c_str(), &FileIconSRV);

	return true;
}



void FRenderer::SetConstantBuffers()
{
	ID3D11Buffer* CBs[2] = { FrameConstantBuffer, ObjectConstantBuffer };
	DeviceContext->VSSetConstantBuffers(0, 2, CBs);
	DeviceContext->PSSetConstantBuffers(0, 2, CBs);
}

void FRenderer::BeginFrame()
{
	if (GUINewFrame) GUINewFrame();
	if (GUIUpdate) GUIUpdate();

	constexpr float ClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	if (RenderTargetView) DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);
	if (DepthStencilView) DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	ID3D11RenderTargetView* ActiveRTV = RenderTargetView;
	ID3D11DepthStencilView* ActiveDSV = DepthStencilView;
	D3D11_VIEWPORT ActiveVP = Viewport;

	if (bUseLevelRenderTargetOverride)
	{
		ActiveRTV = LevelRenderTargetView;
		ActiveDSV = LevelDepthStencilView;
		ActiveVP = LevelViewport;
		if (ActiveRTV && ActiveRTV != RenderTargetView) DeviceContext->ClearRenderTargetView(ActiveRTV, ClearColor);
		if (ActiveDSV && ActiveDSV != DepthStencilView) DeviceContext->ClearDepthStencilView(ActiveDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	DeviceContext->OMSetRenderTargets(1, &ActiveRTV, ActiveDSV);
	DeviceContext->RSSetViewports(1, &ActiveVP);

	ClearCommandList();
}

void FRenderer::ClearCommandList()
{
	PrevCommandCount = CommandList.size();
	CommandList.clear();
	CommandList.reserve(PrevCommandCount);	
}

void FRenderer::RebuildAllLayerInstanceBuffers(TArray<FRenderCommand>& InCommandList)
{
	if (!bInstanceBufferDirty) return;

	// 전체 인스턴스 수 계산
	uint32 TotalInstances = static_cast<uint32>(InCommandList.size());
	if (TotalInstances == 0) return;
	EnsureInstanceBufferCapacity(TotalInstances);

	//  모든 레이어의 캐시 클리어
	for (auto& [Layer, Batches] : LayerCachedBatches)
		Batches.clear();

	//한 번의 Map으로 전체 인스턴스 데이터 기록
	D3D11_MAPPED_SUBRESOURCE Mapped;
	if (FAILED(DeviceContext->Map(InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
		return;

	FInstanceData* InstanceData = static_cast<FInstanceData*>(Mapped.pData);
	uint32 GlobalOffset = 0;

	// 커맨드 리스트는 이미 (RenderLayer, SortKey) 순으로 정렬됨
	auto it = InCommandList.begin();
	while (it != InCommandList.end())
	{
		ERenderLayer CurrentLayer = it->RenderLayer;
		auto& CachedBatches = LayerCachedBatches[CurrentLayer];

		// 이 레이어의 커맨드들을 배치로 묶으며 인스턴스 데이터 기록
		while (it != InCommandList.end() && it->RenderLayer == CurrentLayer)
		{
			auto batchStart = it;
			auto batchEnd = it;
			while (batchEnd != InCommandList.end() &&
				batchEnd->RenderLayer == CurrentLayer &&
				batchEnd->SortKey == batchStart->SortKey)
			{
				++batchEnd;
			}

			uint32 InstanceCount = static_cast<uint32>(std::distance(batchStart, batchEnd));
			FRenderCommand& Cmd = *batchStart;

			if (!Cmd.MeshData || (Cmd.MeshData->Vertices.empty() && Cmd.MeshData->Indices.empty()))
			{
				it = batchEnd;
				continue;
			}

			// 인스턴스 데이터 복사
			uint32 Index = 0;
			for (auto curr = batchStart; curr != batchEnd; ++curr)
			{
				InstanceData[GlobalOffset + Index].World = curr->WorldMatrix;
				InstanceData[GlobalOffset + Index].ObjectID = curr->ObjectID;
				Index++;
			}

			// 캐시에 배치 정보 저장
			FCachedBatch NewBatch;
			NewBatch.Material = Cmd.Material;
			NewBatch.MeshData = Cmd.MeshData;
			NewBatch.FirstIndex = Cmd.FirstIndex;
			NewBatch.IndexCount = Cmd.IndexCount;
			NewBatch.InstanceCount = InstanceCount;
			NewBatch.InstanceBufferOffset = GlobalOffset; 
			NewBatch.WorldMatrix = Cmd.WorldMatrix;
			CachedBatches.push_back(NewBatch);

			GlobalOffset += InstanceCount;
			it = batchEnd;
		}
	}

	DeviceContext->Unmap(InstanceBuffer, 0);
}

void FRenderer::EndFrame()
{
	if (RenderTargetView)
	{
		DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
		DeviceContext->RSSetViewports(1, &Viewport);
	}

	if (GUIRender) GUIRender();

	UINT SyncInterval = bVSyncEnabled ? 1 : 0;
	HRESULT Hr = SwapChain->Present(SyncInterval, 0);
	if (Hr == DXGI_STATUS_OCCLUDED) bSwapChainOccluded = true;

	if (GUIPostPresent) GUIPostPresent();
}

void FRenderer::SubmitCommands(const FRenderCommandQueue& InQueue)
{
	ViewMatrix = InQueue.ViewMatrix;
	ProjectionMatrix = InQueue.ProjectionMatrix;

	for (const auto& Cmd : InQueue.Commands)
	{
		if (Cmd.MeshData && Cmd.MeshData->IsDirty())
		{
			Cmd.MeshData->UpdateVertexAndIndexBuffer(Device, DeviceContext); // 여기도 수정!
		}
		AddCommand(Cmd);
	}
}

void FRenderer::ExecuteCommandQueue(const FRenderCommandQueue& InQueue)
{
	LocalCommandList.clear();

	if (LocalCommandList.capacity() < InQueue.Commands.size())
	{
		LocalCommandList.reserve(InQueue.Commands.size());
	}
	for (const FRenderCommand& Cmd : InQueue.Commands)
	{
		if (Cmd.MeshData)
		{
		
			if (Cmd.MeshData->IsDirty())
			{
				Cmd.MeshData->UpdateVertexAndIndexBuffer(Device, DeviceContext);
			}
		}
		AddCommand(LocalCommandList, Cmd);
	}

	if (PrevCommandCount != InQueue.Commands.size())
	{
		bInstanceBufferDirty = true;
	}
	PrevCommandCount = InQueue.Commands.size();
	ExecuteCommands(LocalCommandList, InQueue.ViewMatrix, InQueue.ProjectionMatrix);

}

void FRenderer::SetViewport(D3D11_VIEWPORT* Viewport)
{
	DeviceContext->RSSetViewports(1, Viewport);
}

void FRenderer::AddCommand(const FRenderCommand& Command)
{
	CommandList.push_back(Command);
	FRenderCommand& Added = CommandList.back();
	if (!Added.Material) Added.Material = DefaultMaterial.get();
	Added.SortKey = FRenderCommand::MakeSortKey(Added.Material, Added.MeshData, Added.FirstIndex);
}

void FRenderer::AddCommand(TArray<FRenderCommand>& CommandBuffer, const FRenderCommand& Command)
{
	CommandBuffer.push_back(Command);
	FRenderCommand& Added = CommandBuffer.back();
	if (!Added.Material) Added.Material = DefaultMaterial.get();
	Added.SortKey = FRenderCommand::MakeSortKey(Added.Material, Added.MeshData, Added.FirstIndex);
}

void FRenderer::ExecuteCommands()
{
	ExecuteCommands(CommandList, ViewMatrix, ProjectionMatrix);
}

void FRenderer::ExecuteCommands(TArray<FRenderCommand>& InCommandList, const FMatrix& InViewMatrix, const FMatrix& InProjectionMatrix)
{
	if (ViewMatrix != InViewMatrix || ProjectionMatrix != InProjectionMatrix)
	{
		bInstanceBufferDirty = true;
	}
	ViewMatrix = InViewMatrix;
	ProjectionMatrix = InProjectionMatrix;

	std::sort(InCommandList.begin(), InCommandList.end(),
		[](const FRenderCommand& A, const FRenderCommand& B) {
			if (A.RenderLayer != B.RenderLayer) return A.RenderLayer < B.RenderLayer;
			return A.SortKey < B.SortKey;
		});

	SetConstantBuffers();
	UpdateFrameConstantBuffer();

	if (bInstanceBufferDirty)
	{
		// ★ 모든 레이어를 한 번에 인스턴스 버퍼에 기록
		RebuildAllLayerInstanceBuffers(InCommandList);
	}

	// 각 패스는 캐시된 배치만 사용 (버퍼 재기록 안 함)
	ExecuteRenderPass(InCommandList, ERenderLayer::Default);
	ExecuteRenderPass(InCommandList, ERenderLayer::Translucent);
	ClearDepthBuffer();
	ExecuteRenderPass(InCommandList, ERenderLayer::Overlay);
	bInstanceBufferDirty = false;
}

void FRenderer::ExecuteRenderPass(ERenderLayer InRenderLayer)
{
	ExecuteRenderPass(CommandList, InRenderLayer);
}
void FRenderer::ExecuteRenderPass(TArray<FRenderCommand>& InCommandList, ERenderLayer InRenderLayer)
{
	if (InCommandList.empty()) return;

	FMaterial* CurrentMaterial = nullptr;
	FMeshData* CurrentMesh = nullptr;
	D3D11_PRIMITIVE_TOPOLOGY CurrentMeshTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

	ID3D11ShaderResourceView* FontSRV = TextRenderer.GetAtlasSRV();
	ID3D11SamplerState* FontSampler = TextRenderer.GetAtlasSampler();
	ID3D11ShaderResourceView* SubUVSRV = SubUVRenderer.GetTextureSRV();
	ID3D11SamplerState* SubUVSampler = SubUVRenderer.GetSamplerState();

	auto& CachedBatches = LayerCachedBatches[InRenderLayer];
	if (CachedBatches.empty()) return;

	RenderStateManager->RebindState();

	UINT Stride = sizeof(FInstanceData);
	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(1, 1, &InstanceBuffer, &Stride, &Offset);

	for (const FCachedBatch& Batch : CachedBatches)
	{
		if (!Batch.MeshData) continue;

		if (Batch.Material != CurrentMaterial)
		{
			Batch.Material->Bind(DeviceContext);
			if (Batch.Material->GetOriginName() != "M_EditorGrid")
			{
				InstancedVertexShader->Bind(DeviceContext);
			}
			RenderStateManager->BindState(Batch.Material->GetRasterizerState());
			RenderStateManager->BindState(Batch.Material->GetDepthStencilState());
			RenderStateManager->BindState(Batch.Material->GetBlendState());
			CurrentMaterial = Batch.Material;

			if (CurrentMaterial->GetOriginName() == "M_Font") {
				DeviceContext->PSSetShaderResources(0, 1, &FontSRV);
				DeviceContext->PSSetSamplers(0, 1, &FontSampler);
			}
			else if (CurrentMaterial->GetOriginName() == "M_SubUV") {
				DeviceContext->PSSetShaderResources(0, 1, &SubUVSRV);
				DeviceContext->PSSetSamplers(0, 1, &SubUVSampler);
			}
			else {
				DeviceContext->PSSetSamplers(0, 1, &NormalSampler);
			}
		}

		if (Batch.MeshData != CurrentMesh)
		{
			Batch.MeshData->Bind(DeviceContext);
			CurrentMesh = Batch.MeshData;
		}

		D3D11_PRIMITIVE_TOPOLOGY DesiredTopology = static_cast<D3D11_PRIMITIVE_TOPOLOGY>(CurrentMesh->Topology);
		if (DesiredTopology != CurrentMeshTopology)
		{
			DeviceContext->IASetPrimitiveTopology(DesiredTopology);
			CurrentMeshTopology = DesiredTopology;
		}

		UpdateObjectConstantBuffer(Batch.WorldMatrix);
		if (Batch.IndexCount > 0)
			DeviceContext->DrawIndexedInstanced(Batch.IndexCount, Batch.InstanceCount, Batch.FirstIndex, 0, Batch.InstanceBufferOffset);
		else if (!Batch.MeshData->Indices.empty())
			DeviceContext->DrawIndexedInstanced(static_cast<UINT>(Batch.MeshData->Indices.size()), Batch.InstanceCount, 0, 0, Batch.InstanceBufferOffset);
		else if (!Batch.MeshData->Vertices.empty())
			DeviceContext->DrawInstanced(static_cast<UINT>(Batch.MeshData->Vertices.size()), Batch.InstanceCount, 0, Batch.InstanceBufferOffset);
	}
}

void FRenderer::ClearCachedBatches()
{
	for (auto& [Layer, Batches] : LayerCachedBatches)
	{
		Batches.clear();
	}


	bInstanceBufferDirty = true;
	PrevCommandCount = -1;
	CommandList.clear();
	LocalCommandList.clear();
}

void FRenderer::RenderPickingPass()
{
	if (!PickingRTV || !PickingDSV || !PickingMaterial) return;

	// 피킹 타겟 클리어 (배경은 0 = 선택 안 됨)
	const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	DeviceContext->ClearRenderTargetView(PickingRTV, ClearColor);
	DeviceContext->ClearDepthStencilView(PickingDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
	D3D11_VIEWPORT ActiveVP = {};
	ActiveVP.TopLeftX = 0.0f;
	ActiveVP.TopLeftY = 0.0f;
	ActiveVP.Width = bUseLevelRenderTargetOverride ? LevelViewport.Width : Viewport.Width;
	ActiveVP.Height = bUseLevelRenderTargetOverride ? LevelViewport.Height : Viewport.Height;
	ActiveVP.MinDepth = 0.0f;
	ActiveVP.MaxDepth = 1.0f;

	DeviceContext->OMSetRenderTargets(1, &PickingRTV, PickingDSV);
	DeviceContext->RSSetViewports(1, &ActiveVP);
	SetConstantBuffers();
	UpdateFrameConstantBuffer();

	PickingMaterial->Bind(DeviceContext);
	InstancedVertexShader->Bind(DeviceContext);
	RenderStateManager->BindState(PickingMaterial->GetRasterizerState());
	RenderStateManager->BindState(PickingMaterial->GetDepthStencilState());
	RenderStateManager->BindState(PickingMaterial->GetBlendState());

	UINT Stride = sizeof(FInstanceData);
	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(1, 1, &InstanceBuffer, &Stride, &Offset);
	const auto& PickBatches = LayerCachedBatches[ERenderLayer::Default];
	if (PickBatches.empty())
	{
		// 캐시 비어있으면 렌더타겟 복원 후 리턴
		ID3D11RenderTargetView* ActiveRTV = bUseLevelRenderTargetOverride ? LevelRenderTargetView : RenderTargetView;
		ID3D11DepthStencilView* ActiveDepth = bUseLevelRenderTargetOverride ? LevelDepthStencilView : DepthStencilView;
		DeviceContext->OMSetRenderTargets(1, &ActiveRTV, ActiveDepth);
		return;
	}

	// Default와 Overlay 레이어 모두 피킹 대상에 포함 (빌보드 선택 지원)
	UE_LOG("[PickingPass] VP=%.0fx%.0f, Default=%d, Overlay=%d, Override=%d",
		ActiveVP.Width, ActiveVP.Height,
		static_cast<int>(LayerCachedBatches[ERenderLayer::Default].size()),
		static_cast<int>(LayerCachedBatches[ERenderLayer::Overlay].size()),
		static_cast<int>(bUseLevelRenderTargetOverride));
	ERenderLayer PickLayers[] = { ERenderLayer::Default, ERenderLayer::Translucent, ERenderLayer::Overlay };
	for (ERenderLayer Layer : PickLayers)
	{
		for (const FCachedBatch& Batch : LayerCachedBatches[Layer])
		{
			if (!Batch.MeshData) continue;
			Batch.MeshData->Bind(DeviceContext);
			DeviceContext->IASetPrimitiveTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(Batch.MeshData->Topology));

			if (Batch.IndexCount > 0)
				DeviceContext->DrawIndexedInstanced(Batch.IndexCount, Batch.InstanceCount, Batch.FirstIndex, 0, Batch.InstanceBufferOffset);
			else if (!Batch.MeshData->Indices.empty())
				DeviceContext->DrawIndexedInstanced(static_cast<UINT>(Batch.MeshData->Indices.size()), Batch.InstanceCount, 0, 0, Batch.InstanceBufferOffset);
		}
	}
	ID3D11RenderTargetView* ActiveRTV = bUseLevelRenderTargetOverride ? LevelRenderTargetView : RenderTargetView;
	ID3D11DepthStencilView* ActiveDepth = bUseLevelRenderTargetOverride ? LevelDepthStencilView : DepthStencilView;

	DeviceContext->OMSetRenderTargets(1, &ActiveRTV, ActiveDepth);
	D3D11_VIEWPORT MainVP = bUseLevelRenderTargetOverride ? LevelViewport : Viewport;
	DeviceContext->RSSetViewports(1, &MainVP);
}
uint32 FRenderer::ReadPixelID(int32 ScreenX, int32 ScreenY)
{
	int32 MaxWidth = bUseLevelRenderTargetOverride ? static_cast<int32>(LevelViewport.Width) : static_cast<int32>(Viewport.Width);
	int32 MaxHeight = bUseLevelRenderTargetOverride ? static_cast<int32>(LevelViewport.Height) : static_cast<int32>(Viewport.Height);

	UE_LOG("[ReadPixelID] Screen=(%d,%d), Max=(%d,%d), Override=%d, PickTex=%p",
		ScreenX, ScreenY, MaxWidth, MaxHeight,
		static_cast<int>(bUseLevelRenderTargetOverride), PickingTexture);

	if (!PickingTexture || !PickingStagingTexture || ScreenX < 0 || ScreenY < 0 || ScreenX >= MaxWidth || ScreenY >= MaxHeight)
	{
		UE_LOG("[ReadPixelID] REJECTED: out of bounds or null texture");
		return 0;
	}


	D3D11_BOX Box;
	Box.left = ScreenX;
	Box.right = ScreenX + 1;
	Box.top = ScreenY;
	Box.bottom = ScreenY + 1;
	Box.front = 0;
	Box.back = 1;

	DeviceContext->CopySubresourceRegion(PickingStagingTexture, 0, 0, 0, 0, PickingTexture, 0, &Box);

	// CPU로 읽어오기
	D3D11_MAPPED_SUBRESOURCE Mapped;
	if (SUCCEEDED(DeviceContext->Map(PickingStagingTexture, 0, D3D11_MAP_READ, 0, &Mapped)))
	{
		uint32 PickedID = *static_cast<uint32*>(Mapped.pData);
		DeviceContext->Unmap(PickingStagingTexture, 0);
		return PickedID;
	}
	return 0;
}
void FRenderer::ClearDepthBuffer()
{
	if (LevelDepthStencilView) DeviceContext->ClearDepthStencilView(LevelDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

FVector FRenderer::GetCameraPosition() const
{
	return GetCameraWorldPositionFromViewMatrix(ViewMatrix);
}

void FRenderer::EnsureInstanceBufferCapacity(uint32 Count)
{
	if (InstanceBufferCapacity >= Count)
	{
		return;
	}

	if (InstanceBuffer)
	{
		InstanceBuffer->Release();
		InstanceBuffer = nullptr;
	}
	InstanceBufferCapacity = Count * 1.5f;
	if (InstanceBufferCapacity < 128)
	{
		InstanceBufferCapacity = 128;
	}

	D3D11_BUFFER_DESC Desc = {};
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;// 인스턴스 버퍼는 버텍스 버퍼로 바인딩됨
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	Desc.StructureByteStride = 0;
	Desc.ByteWidth = InstanceBufferCapacity* sizeof(FInstanceData);
	HRESULT hr = Device->CreateBuffer(&Desc, nullptr, &InstanceBuffer);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"InstanceBuffer Create Failed.", nullptr, 0);
		InstanceBufferCapacity = 0; // 실패 시 용량 초기화
	}
}

bool FRenderer::CreateConstantBuffers()
{
	D3D11_BUFFER_DESC Desc = {};
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	Desc.ByteWidth = sizeof(FFrameConstantBuffer);
	if (FAILED(Device->CreateBuffer(&Desc, nullptr, &FrameConstantBuffer))) return false;

	Desc.ByteWidth = sizeof(FObjectConstantBuffer);
	return SUCCEEDED(Device->CreateBuffer(&Desc, nullptr, &ObjectConstantBuffer));
}

void FRenderer::UpdateFrameConstantBuffer()
{
	FFrameConstantBuffer CBData;
	CBData.View = ViewMatrix.GetTransposed();
	CBData.Projection = ProjectionMatrix.GetTransposed();
	CBData.TotalTime = GEngine ? static_cast<float>(GEngine->GetTimer()->GetTotalTime()) : 0.0f;
	D3D11_MAPPED_SUBRESOURCE Mapped;
	if (SUCCEEDED(DeviceContext->Map(FrameConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
	{
		memcpy(Mapped.pData, &CBData, sizeof(CBData));
		DeviceContext->Unmap(FrameConstantBuffer, 0);
	}
}

void FRenderer::UpdateObjectConstantBuffer(const FMatrix& WorldMatrix)
{
	FObjectConstantBuffer CBData;
	CBData.World = WorldMatrix.GetTransposed();
	D3D11_MAPPED_SUBRESOURCE Mapped;
	if (SUCCEEDED(DeviceContext->Map(ObjectConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
	{
		memcpy(Mapped.pData, &CBData, sizeof(CBData));
		DeviceContext->Unmap(ObjectConstantBuffer, 0);
	}
}

bool FRenderer::CreateTextureFromSTB(ID3D11Device* Device, const wchar_t* FilePath, ID3D11ShaderResourceView** OutSRV)
{
	int W, H, C;
	FILE* f = nullptr;
	_wfopen_s(&f, FilePath, L"rb");
	unsigned char* Data = nullptr;
	if (f)
	{
		Data = stbi_load_from_file(f, &W, &H, &C, 4);
		fclose(f);
	}
	if (!Data) return false;

	D3D11_TEXTURE2D_DESC Desc = {};
	Desc.Width = W; Desc.Height = H; Desc.MipLevels = 1; Desc.ArraySize = 1;
	Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; Desc.SampleDesc.Count = 1;
	Desc.Usage = D3D11_USAGE_DEFAULT; Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA InitData = { Data, static_cast<UINT>(W * 4), 0 };
	ID3D11Texture2D* Tex = nullptr;
	HRESULT hr = Device->CreateTexture2D(&Desc, &InitData, &Tex);
	stbi_image_free(Data);
	if (FAILED(hr)) return false;

	hr = Device->CreateShaderResourceView(Tex, nullptr, OutSRV);
	Tex->Release();
	return SUCCEEDED(hr);
}

bool FRenderer::InitOutlineResources()
{
	if (StencilWriteState && StencilTestState && OutlinePS) return true;

	D3D11_DEPTH_STENCIL_DESC WriteDesc = {};
	WriteDesc.DepthEnable = TRUE; WriteDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; WriteDesc.DepthFunc = D3D11_COMPARISON_LESS;
	WriteDesc.StencilEnable = TRUE; WriteDesc.StencilReadMask = 0xFF; WriteDesc.StencilWriteMask = 0xFF;
	WriteDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE; WriteDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
	WriteDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE; WriteDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	WriteDesc.BackFace = WriteDesc.FrontFace;
	if (FAILED(Device->CreateDepthStencilState(&WriteDesc, &StencilWriteState))) return false;

	D3D11_DEPTH_STENCIL_DESC TestDesc = {};
	TestDesc.DepthEnable = FALSE; TestDesc.StencilEnable = TRUE; TestDesc.StencilReadMask = 0xFF; TestDesc.StencilWriteMask = 0xFF;
	TestDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP; TestDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	TestDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP; TestDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
	TestDesc.BackFace = TestDesc.FrontFace;
	if (FAILED(Device->CreateDepthStencilState(&TestDesc, &StencilTestState))) return false;

	FString PSPath = FPaths::ToAbsolutePath("Engine/Shaders/OutlinePixelShader.hlsl");
	OutlinePS = FShaderMap::Get().GetOrCreatePixelShader(Device, FPaths::ToWide(PSPath).c_str());
	return OutlinePS != nullptr;
}

void FRenderer::RenderOutline(FMeshData* Mesh, const FMatrix& WorldMatrix, float OutlineScale)
{
	if (!Mesh || !InitOutlineResources()) return;
	Mesh->UpdateVertexAndIndexBuffer(Device, DeviceContext);
	Mesh->Bind(DeviceContext);

	ID3D11RenderTargetView* ActiveRTV = bUseLevelRenderTargetOverride ? LevelRenderTargetView : RenderTargetView;
	ID3D11DepthStencilView* ActiveDSV = bUseLevelRenderTargetOverride ? LevelDepthStencilView : DepthStencilView;

	DeviceContext->OMSetRenderTargets(0, nullptr, ActiveDSV);
	DeviceContext->OMSetDepthStencilState(StencilWriteState, 1);
	UpdateObjectConstantBuffer(WorldMatrix);

	DeviceContext->PSSetShader(nullptr, nullptr, 0);
	DeviceContext->DrawIndexed(static_cast<UINT>(Mesh->Indices.size()), 0, 0);

	DeviceContext->OMSetRenderTargets(1, &ActiveRTV, ActiveDSV);
	DeviceContext->OMSetDepthStencilState(StencilTestState, 1);
	UpdateObjectConstantBuffer(FMatrix::MakeScale(OutlineScale) * WorldMatrix);
	OutlinePS->Bind(DeviceContext);
	DeviceContext->DrawIndexed(static_cast<UINT>(Mesh->Indices.size()), 0, 0);

	ShaderManager.Bind(DeviceContext);
	DeviceContext->OMSetDepthStencilState(nullptr, 0);
}

void FRenderer::DrawLine(const FVector& Start, const FVector& End, const FVector4& Color)
{
	LineVertices.push_back({ Start, Color, FVector::ZeroVector });
	LineVertices.push_back({ End, Color, FVector::ZeroVector });
}

void FRenderer::DrawCube(const FVector& Center, const FVector& BoxExtent, const FVector4& Color)
{
	FVector v[8] = {
		Center + FVector(-BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z), Center + FVector(-BoxExtent.X, -BoxExtent.Y, BoxExtent.Z),
		Center + FVector(-BoxExtent.X, BoxExtent.Y, -BoxExtent.Z), Center + FVector(-BoxExtent.X, BoxExtent.Y, BoxExtent.Z),
		Center + FVector(BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z), Center + FVector(BoxExtent.X, -BoxExtent.Y, BoxExtent.Z),
		Center + FVector(BoxExtent.X, BoxExtent.Y, -BoxExtent.Z), Center + FVector(BoxExtent.X, BoxExtent.Y, BoxExtent.Z)
	};
	DrawLine(v[0], v[4], Color); DrawLine(v[4], v[6], Color); DrawLine(v[6], v[2], Color); DrawLine(v[2], v[0], Color);
	DrawLine(v[1], v[5], Color); DrawLine(v[5], v[7], Color); DrawLine(v[7], v[3], Color); DrawLine(v[3], v[1], Color);
	DrawLine(v[0], v[1], Color); DrawLine(v[4], v[5], Color); DrawLine(v[6], v[7], Color); DrawLine(v[2], v[3], Color);
}

void FRenderer::DrawOverlayRect(float X, float Y, float Width, float Height, const FVector4& Color, int32 ViewportWidth, int32 ViewportHeight, bool bUpdateMatrix)
{
	if (!OverlayColorMaterial || Width <= 0.0f || Height <= 0.0f)
	{
		return;
	}

	FMeshData QuadMesh;
	QuadMesh.Topology = EMeshTopology::EMT_TriangleList;

	FPrimitiveVertex V0, V1, V2, V3;
	V0.Position = FVector(0.0f, 0.0f, Height);
	V1.Position = FVector(0.0f, Width, Height);
	V2.Position = FVector(0.0f, Width, 0.0f);
	V3.Position = FVector(0.0f, 0.0f, 0.0f);
	V0.Color = V1.Color = V2.Color = V3.Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	V0.Normal = V1.Normal = V2.Normal = V3.Normal = FVector(0.0f, 0.0f, 1.0f);

	QuadMesh.Vertices = { V0, V1, V2, V3 };
	QuadMesh.Indices = { 0, 1, 2, 0, 2, 3 };

	OverlayColorMaterial->SetParameterData("BaseColor", &Color, 16);

	const float WorldY = X - static_cast<float>(ViewportWidth) * 0.5f;
	const float WorldZ = static_cast<float>(ViewportHeight) * 0.5f - Y - Height;
	const FMatrix WorldMatrix = FMatrix::MakeTranslation(FVector(0.0f, WorldY, WorldZ));

	// 최적화: bUpdateMatrix 전달
	DrawImmediateMesh(&QuadMesh, OverlayColorMaterial.get(), WorldMatrix, ViewportWidth, ViewportHeight, bUpdateMatrix);
}
void FRenderer::DrawOverlayRectOutline(float X, float Y, float Width, float Height, const FVector4& Color, int32 ViewportWidth, int32 ViewportHeight, bool bUpdateMatrix)
{
	constexpr float Thickness = 1.0f;
	// 첫 번째 사각형만 bUpdateMatrix를 따르고, 나머지는 이미 설정된 행렬을 재사용하도록 false 전달
	DrawOverlayRect(X, Y, Width, Thickness, Color, ViewportWidth, ViewportHeight, bUpdateMatrix);
	DrawOverlayRect(X, Y + Height - Thickness, Width, Thickness, Color, ViewportWidth, ViewportHeight, false);
	DrawOverlayRect(X, Y, Thickness, Height, Color, ViewportWidth, ViewportHeight, false);
	DrawOverlayRect(X + Width - Thickness, Y, Thickness, Height, Color, ViewportWidth, ViewportHeight, false);
}
void FRenderer::DrawOverlayText(const FString& Text, float X, float Y, const FVector4& Color, int32 ViewportWidth, int32 ViewportHeight, bool bUpdateMatrix)
{
	if (Text.empty()) return;

	FMeshData TextMesh;
	if (!TextRenderer.BuildTextMesh(Text, TextMesh)) return;

	TextMesh.UpdateLocalBound();
	FMaterial* FontMaterial = TextRenderer.GetFontMaterial();
	if (!FontMaterial) return;

	FontMaterial->SetParameterData("TextColor", &Color, 16);

	constexpr float TextScale = 16.0f;
	const FVector Min = TextMesh.GetMinCoord();
	const FVector Max = TextMesh.GetMaxCoord();
	const float WorldY = X - (float)ViewportWidth * 0.5f - Min.Y * TextScale;
	const float WorldZ = (float)ViewportHeight * 0.5f - Y - Max.Z * TextScale;

	const FMatrix WorldMatrix =
		FMatrix::MakeScale(FVector(1.0f, TextScale, TextScale)) *
		FMatrix::MakeTranslation(FVector(0.0f, WorldY, WorldZ));

	// bUpdateMatrix를 전달하여 불필요한 GPU 행렬 업데이트 방지
	DrawImmediateMesh(&TextMesh, FontMaterial, WorldMatrix, ViewportWidth, ViewportHeight, bUpdateMatrix);
}

void FRenderer::ExecuteLineCommands()
{
	if (LineVertices.empty()) return;
	ShaderManager.Bind(DeviceContext);
	DefaultMaterial->Bind(DeviceContext);
	RenderStateManager->BindState(DefaultMaterial->GetRasterizerState());
	RenderStateManager->BindState(DefaultMaterial->GetDepthStencilState());
	RenderStateManager->BindState(DefaultMaterial->GetBlendState());
	UINT Size = static_cast<UINT>(LineVertices.size() * sizeof(FPrimitiveVertex));
	if (LineVertexBuffer && LineVertexBufferSize < Size) { LineVertexBuffer->Release(); LineVertexBuffer = nullptr; }
	if (!LineVertexBuffer)
	{
		D3D11_BUFFER_DESC Desc = { Size, D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0 };
		Device->CreateBuffer(&Desc, nullptr, &LineVertexBuffer);
		LineVertexBufferSize = Size;
	}
	D3D11_MAPPED_SUBRESOURCE Mapped;
	if (SUCCEEDED(DeviceContext->Map(LineVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
	{
		memcpy(Mapped.pData, LineVertices.data(), Size);
		DeviceContext->Unmap(LineVertexBuffer, 0);
	}
	UINT Stride = sizeof(FPrimitiveVertex), Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &LineVertexBuffer, &Stride, &Offset);
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	UpdateObjectConstantBuffer(FMatrix::Identity);
	DeviceContext->Draw(static_cast<UINT>(LineVertices.size()), 0);
	DeviceContext->OMSetDepthStencilState(nullptr, 0);
	LineVertices.clear();
}

void FRenderer::DrawImmediateMesh(FMeshData* MeshData, FMaterial* Material, const FMatrix& WorldMatrix, int32 ViewportWidth, int32 ViewportHeight, bool bUpdateMatrix)
{
	if (!MeshData || !Material || ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		return;
	}

	if (!MeshData->UpdateVertexAndIndexBuffer(Device, DeviceContext)) return;

	const FMatrix PrevView = ViewMatrix;
	const FMatrix PrevProjection = ProjectionMatrix;
	ViewMatrix = FMatrix::Identity;
	ProjectionMatrix = FMatrix::MakeOrthographicLH(static_cast<float>(ViewportWidth), static_cast<float>(ViewportHeight), 0.0f, 1.0f);

	SetConstantBuffers();
	UpdateFrameConstantBuffer();
	Material->Bind(DeviceContext);
	RenderStateManager->BindState(Material->GetRasterizerState());
	RenderStateManager->BindState(Material->GetDepthStencilState());
	RenderStateManager->BindState(Material->GetBlendState());

	if (Material->GetOriginName() == "M_Font")
	{
		ID3D11ShaderResourceView* FontSRV = TextRenderer.GetAtlasSRV();
		ID3D11SamplerState* FontSampler = TextRenderer.GetAtlasSampler();
		DeviceContext->PSSetShaderResources(0, 1, &FontSRV);
		DeviceContext->PSSetSamplers(0, 1, &FontSampler);
	}
	else
	{
		DeviceContext->PSSetSamplers(0, 1, &NormalSampler);
	}

	MeshData->Bind(DeviceContext);
	DeviceContext->IASetPrimitiveTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(MeshData->Topology));
	UpdateObjectConstantBuffer(WorldMatrix);

	if (!MeshData->Indices.empty())
	{
		DeviceContext->DrawIndexed(static_cast<UINT>(MeshData->Indices.size()), 0, 0);
	}
	else if (!MeshData->Vertices.empty())
	{
		DeviceContext->Draw(static_cast<UINT>(MeshData->Vertices.size()), 0);
	}

	ViewMatrix = PrevView;
	ProjectionMatrix = PrevProjection;
	UpdateFrameConstantBuffer();
}

void FRenderer::Release()
{
	ClearViewportCallbacks();
	ClearLevelRenderTarget();

	if (DeviceContext)
	{
		// Drop pipeline references held by the immediate context before releasing GPU objects.
		DeviceContext->ClearState();
		DeviceContext->Flush();
	}

	TextRenderer.Release();
	SubUVRenderer.Release();
	ShaderManager.Release();

	OutlinePS.reset();
	DefaultMaterial.reset();
	DefaultTextureMaterial.reset();
	OverlayColorMaterial.reset();

	FShaderMap::Get().Clear();
	FMaterialManager::Get().Clear();
	RenderStateManager.reset();
	if (PickingTexture) { PickingTexture->Release(); PickingTexture = nullptr; }
	if (PickingRTV) { PickingRTV->Release(); PickingRTV = nullptr; }
	if (PickingStagingTexture) { PickingStagingTexture->Release(); PickingStagingTexture = nullptr; }
	if (PickingDSV) { PickingDSV->Release(); PickingDSV = nullptr; }
	if (InstanceBuffer) { InstanceBuffer->Release(); InstanceBuffer = nullptr; }
	if (NormalSampler) { NormalSampler->Release(); NormalSampler = nullptr; }
	if (StencilWriteState) { StencilWriteState->Release(); StencilWriteState = nullptr; }
	if (StencilTestState) { StencilTestState->Release(); StencilTestState = nullptr; }
	if (FolderIconSRV) { FolderIconSRV->Release(); FolderIconSRV = nullptr; }
	if (FileIconSRV) { FileIconSRV->Release(); FileIconSRV = nullptr; }
	if (LineVertexBuffer) { LineVertexBuffer->Release(); LineVertexBuffer = nullptr; }
	if (FrameConstantBuffer) { FrameConstantBuffer->Release(); FrameConstantBuffer = nullptr; }
	if (ObjectConstantBuffer) { ObjectConstantBuffer->Release(); ObjectConstantBuffer = nullptr; }
	if (DepthStencilView) { DepthStencilView->Release(); DepthStencilView = nullptr; }
	if (RenderTargetView) { RenderTargetView->Release(); RenderTargetView = nullptr; }
	if (SwapChain) { SwapChain->Release(); SwapChain = nullptr; }
	CPrimitiveBase::ClearCache();
	FAssetManager::Get().ClearCache();
	if (DeviceContext)
	{
		DeviceContext->ClearState();
		DeviceContext->Flush();

		
		DeviceContext->Release();
		DeviceContext = nullptr;
	}

	if (Device) { Device->Release(); Device = nullptr; }
}

bool FRenderer::IsOccluded()
{
	if (bSwapChainOccluded && SwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) return true;
	bSwapChainOccluded = false; return false;
}

void FRenderer::OnResize(int32 W, int32 H)
{
	if (W == 0 || H == 0) return;

	ClearLevelRenderTarget();
	DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	if (RenderTargetView) { RenderTargetView->Release(); RenderTargetView = nullptr; }
	if (DepthStencilView) { DepthStencilView->Release(); DepthStencilView = nullptr; }
	if (PickingTexture) { PickingTexture->Release(); PickingTexture = nullptr; }
	if (PickingRTV) { PickingRTV->Release(); PickingRTV = nullptr; }
	if (PickingStagingTexture) { PickingStagingTexture->Release(); PickingStagingTexture = nullptr; }
	if (PickingDSV) { PickingDSV->Release(); PickingDSV = nullptr; }
	SwapChain->ResizeBuffers(0, W, H, DXGI_FORMAT_UNKNOWN, 0);
	CreateRenderTargetAndDepthStencil(W, H);
	Viewport.Width = static_cast<float>(W); Viewport.Height = static_cast<float>(H);
}
