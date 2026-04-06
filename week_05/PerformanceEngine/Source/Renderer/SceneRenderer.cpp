#include "Renderer/SceneRenderer.h"

#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

#include "Camera/Camera.h"
#include "Graphics/D3D11/D3D11Utils.h"
#include "Graphics/D3D11/D3D11RHI.h"
#include "Picking/PickingSystem.h"
#include "Scene/Scene.h"
#include "StaticMesh/StaticMesh.h"
#include "Visibility/VisibilitySystem.h"
#include <WICTextureLoader.h>
namespace
{


	bool CreateWhiteTexture(ID3D11Device* InDevice, TComPtr<ID3D11ShaderResourceView>& OutTextureView)
	{
		const uint32 WhitePixel = 0xFFFFFFFFu;

		const D3D11_TEXTURE2D_DESC TextureDesc =
		{
			1,
			1,
			1,
			1,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			{ 1, 0 },
			D3D11_USAGE_IMMUTABLE,
			D3D11_BIND_SHADER_RESOURCE,
			0,
			0
		};

		const D3D11_SUBRESOURCE_DATA InitialData =
		{
			&WhitePixel,
			sizeof(uint32),
			0
		};

		TComPtr<ID3D11Texture2D> Texture;
		if (FAILED(InDevice->CreateTexture2D(&TextureDesc, &InitialData, Texture.GetAddressOf())))
		{
			return false;
		}

		return SUCCEEDED(InDevice->CreateShaderResourceView(Texture.Get(), nullptr, OutTextureView.GetAddressOf()));
	}
}

struct alignas(16) FFrameConstants
{
	FMatrix ViewProjection = FMatrix::Identity;
	FVector2 RenderTargetSize = { 1.0f, 1.0f };
	uint32 PrimitiveCount = 0;
	float Padding = 0.0f;
	float CameraPos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct alignas(16) FInstanceIndexConstants
{
	uint32 InstanceIndex = 0;
	float Padding[3] = { 0.0f, 0.0f, 0.0f };
	float Tint[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};
struct alignas(16) FHighlightConstants
{
	uint32 SelectedPrimitiveIndex = static_cast<uint32>(-1);
	float Padding[3] = { 0.0f, 0.0f, 0.0f };
};
struct FSceneRenderer::FResources
{
	TComPtr<ID3D11VertexShader> VertexShader;
	TComPtr<ID3D11PixelShader> PixelShader;
	TComPtr<ID3D11InputLayout> InputLayout;
	TComPtr<ID3D11VertexShader> ImpostorVS;
	TComPtr<ID3D11PixelShader> ImpostorPS;
	// 메쉬 포인터 → 해당 메쉬의 임포스터 아틀라스 SRV
	std::unordered_map<FStaticMesh*, TComPtr<ID3D11ShaderResourceView>> ImpostorAtlasMap;
	TComPtr<ID3D11Buffer> FrameConstantBuffer;
	TComPtr<ID3D11Buffer> ObjectConstantBuffer;
	TComPtr<ID3D11SamplerState> LinearSampler;
	TComPtr<ID3D11ShaderResourceView> WhiteTextureView;
	TComPtr<ID3D11Buffer> InstanceIndexConstantBuffer;
	TComPtr<ID3D11Buffer> HighlightConstantBuffer;
};

FSceneRenderer::FSceneRenderer() = default;
FSceneRenderer::~FSceneRenderer() = default;

uint32 FSceneRenderer::DrawCallCount = 0;

bool FSceneRenderer::Initialize(FD3D11RHI& InRHI)
{
	ID3D11Device* Device = InRHI.GetDevice();
	ID3D11DeviceContext* DeviceContext = InRHI.GetDeviceContext();
	if (Device == nullptr || DeviceContext == nullptr)
	{
		return false;
	}

	Resources = std::make_unique<FResources>();
	if (!Resources)
	{
		return false;
	}

	static constexpr char VertexShaderSource[] = R"(
cbuffer FrameCB : register(b0)
{
    row_major float4x4 ViewProjection;
	float2 RenderTargetSize;
	uint PrimitiveCount;
	float Padding;
};

cbuffer InstanceIndexCB : register(b1)
{
	uint InstanceIndex;
	float3 Padding2;
};
struct InstanceData
{
	float4x4 WorldMatrix;
	float3 Center;
	float Padding1;
	float3 Extents;
	float Padding2;
};
StructuredBuffer<InstanceData> AllInstances : register(t1);
struct VSInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

VSOutput VSMain(VSInput Input)
{
    VSOutput Output;
	float4x4 WorldMat = AllInstances[InstanceIndex].WorldMatrix;
    
	float4 WorldPosition = mul(float4(Input.Position, 1.0f), WorldMat);
    
    Output.Position = mul(WorldPosition, ViewProjection);
    Output.TexCoord = Input.TexCoord;
    return Output;
}
)";

	static constexpr char PixelShaderSource[] = R"(
cbuffer InstanceIndexCB : register(b1)
{
    uint InstanceIndex;
    float3 Padding2;
    float4 Tint;
};
Texture2D DiffuseTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 PSMain(PSInput Input) : SV_Target
{
    return DiffuseTexture.Sample(LinearSampler, Input.TexCoord) * Tint;
}
)";
	const std::string ImpostorShaderSource = R"(
	struct InstanceData { float4x4 WorldMatrix; float3 Center; float Padding1; float3 Extents; float Padding2; };
	Texture2D AlbedoAtlas : register(t0);
	StructuredBuffer<InstanceData> AllInstances : register(t1);
	StructuredBuffer<uint> ImpostorIndices : register(t2);
	SamplerState LinearSampler : register(s0);

	cbuffer FrameCB : register(b0) { row_major float4x4 ViewProj; float2 RTSize; uint PrimCount; float Padding; float4 CameraPos; };

	struct VS_OUT {
		float4 Pos      : SV_POSITION;
		float2 LocalUV  : TEXCOORD0;  
		float2 GridPosF : TEXCOORD1;  
	};

	float2 SignNotZero(float2 v) { return float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0); }
	float2 OctEncode(float3 dir) {
		float l1norm = abs(dir.x) + abs(dir.y) + abs(dir.z);
		float2 res = dir.xy / l1norm;
		if (dir.z < 0.0) res = (1.0 - abs(res.yx)) * SignNotZero(res);
		return res;
	}

	VS_OUT VSMain(uint VertexID : SV_VertexID, uint InstanceID : SV_InstanceID) {
		VS_OUT Out;
		uint RealIndex = ImpostorIndices[InstanceID];
		InstanceData data = AllInstances[RealIndex];

		float3 worldCenter = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), data.WorldMatrix).xyz;
		float3 dirToCameraWorld = normalize(CameraPos.xyz - worldCenter);
		float3 dirToCameraLocal = normalize(mul((float3x3)data.WorldMatrix, dirToCameraWorld));

		float3 upGuide = float3(0, 1, 0);
		if (abs(dirToCameraWorld.y) > 0.999f) { upGuide = float3(0, 0, 1); }
		float3 right = normalize(cross(upGuide, dirToCameraWorld));
		float3 up = normalize(cross(dirToCameraWorld, right));

		float2 quadPos;
		quadPos.x = (VertexID % 2) ? 1.0 : -1.0;
		quadPos.y = (VertexID / 2) ? -1.0 : 1.0;

		Out.LocalUV.x = (VertexID % 2) ? 1.0 : 0.0;
		Out.LocalUV.y = (VertexID / 2) ? 1.0 : 0.0;

		float2 octPos = OctEncode(dirToCameraLocal);
		float2 atlasUV = octPos * 0.5 + 0.5;
		float2 gridPosF = atlasUV * 16.0;
		gridPosF.y = 16.0 - gridPosF.y; 
		Out.GridPosF = gridPosF;

		float radius = max(data.Extents.x, max(data.Extents.y, data.Extents.z));
		float scale = radius * 2.0f;
		if (scale < 0.1f) scale = 2.0f;

		float3 finalWorldPos = worldCenter + (right * quadPos.x * scale) + (up * quadPos.y * scale);
		Out.Pos = mul(float4(finalWorldPos, 1.0), ViewProj);
		
		return Out;
	}

	float4 SampleTile(float2 gridCell, float2 localUV) {
		float2 g = clamp(gridCell, 0.0, 15.0);
		float2 safeUV = clamp(localUV, 0.02, 0.98); 
		float2 uv = (g + safeUV) / 16.0;
		return AlbedoAtlas.Sample(LinearSampler, uv);
	}

	float4 PSMain(VS_OUT In) : SV_Target {
		float2 gf = In.GridPosF - 0.5; 
		float2 g00 = floor(gf);         
		float2 blend = frac(gf);        

		float4 c00 = SampleTile(g00,                    In.LocalUV);
		float4 c10 = SampleTile(g00 + float2(1.0, 0.0), In.LocalUV);
		float4 c01 = SampleTile(g00 + float2(0.0, 1.0), In.LocalUV);
		float4 c11 = SampleTile(g00 + float2(1.0, 1.0), In.LocalUV);

		float4 color = lerp(lerp(c00, c10, blend.x), lerp(c01, c11, blend.x), blend.y);

		// [핵심 해결책] 배경의 검은색(0,0,0)이 블렌딩되면서 생긴 테두리 어두워짐 현상 제거
		// RGB를 Alpha로 나누어 원래 색상으로 강제 복원합니다.
		if (color.a > 0.001f) {
			color.rgb /= color.a;
		}

		// 경계선 컷오프를 0.5로 올려서 투명해지기 시작하는 지저분한 픽셀을 과감히 잘라냄
		clip(color.a - 0.5f);
		
		color.rgb = pow(abs(color.rgb), 1.0 / 2.2);

		return color;
	}
	)";
	TComPtr<ID3DBlob> IVSBlob, IPSBlob;
	if (!D3D11Utils::CompileShaderFromSource(ImpostorShaderSource.c_str(), "VSMain", "vs_5_0", IVSBlob, "ImpostorVS") ||
		!D3D11Utils::CompileShaderFromSource(ImpostorShaderSource.c_str(), "PSMain", "ps_5_0", IPSBlob, "ImpostorPS"))
	{
		return false;
	}

	Device->CreateVertexShader(IVSBlob->GetBufferPointer(), IVSBlob->GetBufferSize(), nullptr, Resources->ImpostorVS.GetAddressOf());
	Device->CreatePixelShader(IPSBlob->GetBufferPointer(), IPSBlob->GetBufferSize(), nullptr, Resources->ImpostorPS.GetAddressOf());

	// 아틀라스 텍스처는 씬 로드 후 LoadImpostorAtlases()에서 동적으로 로드

	TComPtr<ID3DBlob> VertexShaderBlob;
	TComPtr<ID3DBlob> PixelShaderBlob;
	if (!D3D11Utils::CompileShaderFromSource(VertexShaderSource, "VSMain", "vs_5_0", VertexShaderBlob, "SceneRenderer vertex shader")
		|| !D3D11Utils::CompileShaderFromSource(PixelShaderSource, "PSMain", "ps_5_0", PixelShaderBlob, "SceneRenderer pixel shader"))
	{
		Resources.reset();
		return false;
	}

	if (FAILED(Device->CreateVertexShader(
		VertexShaderBlob->GetBufferPointer(),
		VertexShaderBlob->GetBufferSize(),
		nullptr,
		Resources->VertexShader.GetAddressOf()))
		|| FAILED(Device->CreatePixelShader(
			PixelShaderBlob->GetBufferPointer(),
			PixelShaderBlob->GetBufferSize(),
			nullptr,
			Resources->PixelShader.GetAddressOf())))
	{
		Resources.reset();
		return false;
	}

	const D3D11_INPUT_ELEMENT_DESC InputElements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, static_cast<UINT>(offsetof(FStaticMeshVertex, Position)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, static_cast<UINT>(offsetof(FStaticMeshVertex, TexCoord)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	if (FAILED(Device->CreateInputLayout(
		InputElements,
		_countof(InputElements),
		VertexShaderBlob->GetBufferPointer(),
		VertexShaderBlob->GetBufferSize(),
		Resources->InputLayout.GetAddressOf())))
	{
		Resources.reset();
		return false;
	}

	if (!D3D11Utils::CreateDynamicConstantBuffer(Device, sizeof(FFrameConstants), Resources->FrameConstantBuffer) ||
		!D3D11Utils::CreateDynamicConstantBuffer(Device, sizeof(FInstanceIndexConstants), Resources->InstanceIndexConstantBuffer))
	{
		Resources.reset();
		return false;
	}
	const D3D11_SAMPLER_DESC SamplerDesc =
	{
		D3D11_FILTER_MIN_MAG_MIP_LINEAR,
		D3D11_TEXTURE_ADDRESS_WRAP,
		D3D11_TEXTURE_ADDRESS_WRAP,
		D3D11_TEXTURE_ADDRESS_WRAP,
		0.0f,
		1,
		D3D11_COMPARISON_ALWAYS,
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		0.0f,
		D3D11_FLOAT32_MAX
	};

	if (FAILED(Device->CreateSamplerState(&SamplerDesc, Resources->LinearSampler.GetAddressOf())))
	{
		Resources.reset();
		return false;
	}

	const D3D11_RASTERIZER_DESC RasterizerDesc =
	{
		D3D11_FILL_SOLID,
		D3D11_CULL_BACK,
		FALSE,
		0,
		0.0f,
		0.0f,
		TRUE,
		FALSE,
		FALSE,
		FALSE
	};

	return true;
}

void FSceneRenderer::Shutdown()
{
	Resources.reset();
}

void FSceneRenderer::LoadImpostorAtlases(FD3D11RHI& InRHI, const FScene& InScene)
{
	if (!Resources) return;

	ID3D11Device* Device = InRHI.GetDevice();
	ID3D11DeviceContext* DeviceContext = InRHI.GetDeviceContext();

	// 기존 아틀라스 맵 초기화
	Resources->ImpostorAtlasMap.clear();

	// 씬의 모든 프리미티브에서 고유 메쉬 수집
	std::unordered_set<FStaticMesh*> UniqueMeshes;
	for (const auto& PrimData : InScene.GetPrimitiveRuntimeData())
	{
		if (PrimData.StaticMesh)
		{
			UniqueMeshes.insert(PrimData.StaticMesh);
		}
	}

	// 각 고유 메쉬에 대해 베이크된 아틀라스 PNG 로드
	for (FStaticMesh* Mesh : UniqueMeshes)
	{
		// 메쉬 소스 경로에서 아틀라스 파일 경로 생성
		// 예: "Data/apple_mid.obj" → "Data/Scene/apple_mid_Impostor_Albedo.png"
		std::filesystem::path SrcPath = Mesh->GetSourcePath();
		std::wstring Stem = SrcPath.stem().wstring(); // 확장자 제거한 파일명

		std::wstring AtlasPath = L"Data/Scene/" + Stem + L"_Impostor_Albedo.png";

		TComPtr<ID3D11ShaderResourceView> AtlasSRV;
		if (SUCCEEDED(DirectX::CreateWICTextureFromFile(Device, DeviceContext, AtlasPath.c_str(), nullptr, AtlasSRV.GetAddressOf())))
		{
			Resources->ImpostorAtlasMap[Mesh] = AtlasSRV;

			char msg[512];
			sprintf_s(msg, "[SceneRenderer] Impostor atlas loaded: %ls\n", AtlasPath.c_str());
			OutputDebugStringA(msg);
		}
		else
		{
			char msg[512];
			sprintf_s(msg, "[SceneRenderer] Impostor atlas NOT found: %ls\n", AtlasPath.c_str());
			OutputDebugStringA(msg);
		}
	}
}

void FSceneRenderer::Render(
	const FD3D11RHI& InRHI,
	const FScene& InScene,
	const FCamera& InCamera,
	const FVisibilityResults& InVisibilityResults,
	const FPickState& InPickState)
{
	if (!Resources) return;

	ID3D11DeviceContext* DeviceContext = InRHI.GetDeviceContext();
	if (DeviceContext == nullptr) return;

	DrawCallCount = 0;

	Prepare(InRHI, InScene, InCamera);

	// 이전 프레임의 Depth Buffer를 통한 가시성 결과를 읽어서 이번 프레임에 렌더링할 프리미티브를 결정
	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	HRESULT hr = DeviceContext->Map(InRHI.StagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &MappedResource);
	uint32* MappedVisibilityData = nullptr;
	if (SUCCEEDED(hr))
	{
		MappedVisibilityData = static_cast<uint32*>(MappedResource.pData);

		if (MappedVisibilityData) InRHI.GetDeviceContext()->Unmap(InRHI.StagingBuffer.Get(), 0);
	}

	// 1st Pass - 이전 프레임의 가시성 결과를 기반으로 렌더링
	TArray<int32> VisibleIndices;
	VisibleIndices.reserve(InVisibilityResults.VisiblePrimitiveIndices.size());

	for (uint32 PrimitiveIndex : InVisibilityResults.VisiblePrimitiveIndices)
	{
		if (MappedVisibilityData && MappedVisibilityData[PrimitiveIndex] > 0)
		{
			VisibleIndices.push_back(PrimitiveIndex);
		}
	}

	RenderPrimitives(InRHI, InScene, InCamera, VisibleIndices, InPickState);

	ID3D11ShaderResourceView* NullSRVs[] = { nullptr, nullptr };
	DeviceContext->VSSetShaderResources(0, 2, NullSRVs);

	ID3D11RenderTargetView* NullRTVs[] = { nullptr };
	DeviceContext->OMSetRenderTargets(1, NullRTVs, nullptr);

	// 이번 프레임의 depth를 기반으로 Hi-Z Mip Chain 생성
	BuildHiZMipChain(InRHI, InScene);

	//TArray<int32> NewlyVisibleIndices;
	//NewlyVisibleIndices.reserve(InvisibleIndices.size());

	//hr = DeviceContext->Map(InRHI.StagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &MappedResource);
	//if (SUCCEEDED(hr))
	//{
	//	uint32* NewVisibilityData = static_cast<uint32*>(MappedResource.pData);
	//	for (uint32 PrimitiveIndex : InvisibleIndices)
	//	{
	//		if (NewVisibilityData && NewVisibilityData[PrimitiveIndex] > 0)
	//		{
	//			NewlyVisibleIndices.push_back(PrimitiveIndex);
	//		}
	//	}
	//	DeviceContext->Unmap(InRHI.StagingBuffer.Get(), 0);
	//}

	//if (NewlyVisibleIndices.size() > 0)
	//{
	//	ID3D11RenderTargetView* RenderTargets[] = { InRHI.GetBackBufferRTV() };
	//	DeviceContext->OMSetRenderTargets(1, RenderTargets, InRHI.GetDepthStencilView());

	//	RenderPrimitives(InRHI, InScene, InCamera, NewlyVisibleIndices, InPickState);
	//}
}

void FSceneRenderer::Prepare(const FD3D11RHI& InRHI, const FScene& InScene, const FCamera& InCamera)
{
	ID3D11DeviceContext* DeviceContext = InRHI.GetDeviceContext();

	ID3D11RenderTargetView* RenderTargets[] = { InRHI.GetBackBufferRTV() };
	const D3D11_VIEWPORT Viewport = InRHI.GetViewport();
	DeviceContext->OMSetRenderTargets(1, RenderTargets, InRHI.GetDepthStencilView());
	DeviceContext->RSSetViewports(1, &Viewport);

	DeviceContext->IASetInputLayout(Resources->InputLayout.Get());
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DeviceContext->VSSetShader(Resources->VertexShader.Get(), nullptr, 0);
	DeviceContext->PSSetShader(Resources->PixelShader.Get(), nullptr, 0);

	DeviceContext->RSSetState(InRHI.GetRasterizerState(D3D11_FILL_SOLID, D3D11_CULL_BACK, FALSE));
	DeviceContext->OMSetDepthStencilState(InRHI.GetDepthStencilState(TRUE, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_LESS_EQUAL), 0);
	DeviceContext->OMSetBlendState(InRHI.GetBlendState(FALSE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL), nullptr, 0xffffffffu);

	ID3D11SamplerState* Samplers[] = { Resources->LinearSampler.Get() };
	DeviceContext->PSSetSamplers(0, 1, Samplers);

	FFrameConstants FrameConstants = {};
	FrameConstants.ViewProjection = InCamera.GetViewMatrix() * InCamera.GetProjectionMatrix();
	FrameConstants.RenderTargetSize = { static_cast<float>(InRHI.GetViewportWidth()), static_cast<float>(InRHI.GetViewportHeight()) };
	FrameConstants.PrimitiveCount = static_cast<uint32>(InScene.GetPrimitiveCount());
	FVector CamLoc = InCamera.GetLocation();
	FrameConstants.CameraPos[0] = CamLoc.X;
	FrameConstants.CameraPos[1] = CamLoc.Y;
	FrameConstants.CameraPos[2] = CamLoc.Z;
	FrameConstants.CameraPos[3] = 1.0f;
	D3D11Utils::UpdateDynamicBuffer(DeviceContext, Resources->FrameConstantBuffer.Get(), FrameConstants);


	ID3D11Buffer* VSConstantBuffers[] = { Resources->FrameConstantBuffer.Get(), Resources->InstanceIndexConstantBuffer.Get() };
	ID3D11Buffer* PSConstantBuffers[] = { nullptr, Resources->InstanceIndexConstantBuffer.Get(), Resources->HighlightConstantBuffer.Get() };
	DeviceContext->VSSetConstantBuffers(0, 2, VSConstantBuffers);
	DeviceContext->PSSetConstantBuffers(0, 3, PSConstantBuffers);

	//Vertex Shader에 AllInstances
	ID3D11ShaderResourceView* InstanceSRVs[] = { nullptr, InRHI.InstanceSRV.Get() };
	DeviceContext->VSSetShaderResources(0, 2, InstanceSRVs);
}

void FSceneRenderer::RenderPrimitives(
	const FD3D11RHI& InRHI,
	const FScene& InScene,
	const FCamera& InCamera,
	const TArray<int32>& VisibleIndices,
	const FPickState& InPickState)
{
	ID3D11DeviceContext* DeviceContext = InRHI.GetDeviceContext();
	const TArray<FScenePrimitiveRuntimeData>& PrimitiveRuntimeData = InScene.GetPrimitiveRuntimeData();

	FVector CameraPos = InCamera.GetLocation();
	TArray<uint32> MeshInstanceIndices;
	std::unordered_map<FStaticMesh*, TArray<uint32>> ImpostorBatches;

	for (uint32 PrimitiveIndex : VisibleIndices)
	{
		if (PrimitiveIndex >= PrimitiveRuntimeData.size()) continue;

		const FScenePrimitiveRuntimeData& PrimitiveData = PrimitiveRuntimeData[PrimitiveIndex];

		// 메쉬 중심점과 카메라 사이의 절대 거리를 구합니다.
		float Distance = FVector::Dist(PrimitiveData.WorldBounds.GetCenter(), CameraPos);

	
		if (Distance < 100.0f)
		{
			MeshInstanceIndices.push_back(PrimitiveIndex);
		}
		else
		{
			ImpostorBatches[PrimitiveData.StaticMesh].push_back(PrimitiveIndex);
		}
	}

	// =========================================================================
	// 2. 일반 메쉬 렌더링 (기존 로직 그대로)
	// =========================================================================
	ID3D11Buffer* CurrentVertexBuffer = nullptr;
	ID3D11Buffer* CurrentIndexBuffer = nullptr;
	ID3D11ShaderResourceView* CurrentTextureView = nullptr;
	const UINT Stride = sizeof(FStaticMeshVertex);
	const UINT Offset = 0;

	for (uint32 PrimitiveIndex : MeshInstanceIndices)
	{
		const FScenePrimitiveRuntimeData& PrimitiveData = PrimitiveRuntimeData[PrimitiveIndex];
		FStaticMesh* StaticMesh = PrimitiveData.StaticMesh;
		if (StaticMesh == nullptr || !StaticMesh->IsValid()) continue;

		ID3D11Buffer* VertexBuffer = StaticMesh->GetVertexBuffer();
		if (VertexBuffer != CurrentVertexBuffer) {
			DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
			CurrentVertexBuffer = VertexBuffer;
		}

		ID3D11Buffer* IndexBuffer = StaticMesh->GetIndexBuffer();
		if (IndexBuffer != CurrentIndexBuffer) {
			DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			CurrentIndexBuffer = IndexBuffer;
		}

		FInstanceIndexConstants IndexData;
		IndexData.InstanceIndex = PrimitiveIndex;
		IndexData.Tint[0] = (PrimitiveData.PrimitiveId == InPickState.SelectedPrimitiveId) ? 1.0f : 1.0f;
		IndexData.Tint[1] = (PrimitiveData.PrimitiveId == InPickState.SelectedPrimitiveId) ? 0.2f : 1.0f;
		IndexData.Tint[2] = (PrimitiveData.PrimitiveId == InPickState.SelectedPrimitiveId) ? 0.2f : 1.0f;
		IndexData.Tint[3] = 1.0f;
		D3D11Utils::UpdateDynamicBuffer(DeviceContext, Resources->InstanceIndexConstantBuffer.Get(), IndexData);

		for (const FStaticMesh::FSection& Section : StaticMesh->GetSections()) {
			ID3D11ShaderResourceView* TextureView = StaticMesh->GetMaterialTexture(Section.MaterialIndex);
			if (TextureView == nullptr) TextureView = Resources->WhiteTextureView.Get();

			if (TextureView != CurrentTextureView) {
				DeviceContext->PSSetShaderResources(0, 1, &TextureView);
				CurrentTextureView = TextureView;
			}
			DeviceContext->DrawIndexed(Section.IndexCount, Section.IndexStart, 0);
			++DrawCallCount;
		}
	}

	// =========================================================================
	// 3. 임포스터 렌더링 (메쉬 묶음별로 따로 그리기)
	// =========================================================================
	if (!ImpostorBatches.empty() && Resources->ImpostorVS)
	{
		ID3D11Buffer* NullVB = nullptr;
		UINT ZeroStride = 0, ZeroOffset = 0;
		DeviceContext->IASetVertexBuffers(0, 1, &NullVB, &ZeroStride, &ZeroOffset);
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		DeviceContext->IASetInputLayout(nullptr);
		DeviceContext->RSSetState(InRHI.GetRasterizerState(D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE));

		DeviceContext->VSSetShader(Resources->ImpostorVS.Get(), nullptr, 0);
		DeviceContext->PSSetShader(Resources->ImpostorPS.Get(), nullptr, 0);

		// 🔥 메쉬 묶음 단위로 루프를 돌면서 각각 그려줍니다.
		for (auto& Pair : ImpostorBatches)
		{
			FStaticMesh* TargetMesh = Pair.first;
			const TArray<uint32>& BatchIndices = Pair.second;
			if (BatchIndices.empty()) continue;

			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = static_cast<UINT>(sizeof(uint32) * BatchIndices.size());
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			desc.StructureByteStride = sizeof(uint32);

			D3D11_SUBRESOURCE_DATA initData = { BatchIndices.data(), 0, 0 };
			TComPtr<ID3D11Buffer> TempIdxBuffer;
			if (FAILED(InRHI.GetDevice()->CreateBuffer(&desc, &initData, TempIdxBuffer.GetAddressOf()))) continue;

			TComPtr<ID3D11ShaderResourceView> TempIdxSRV;
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = static_cast<UINT>(BatchIndices.size());
			InRHI.GetDevice()->CreateShaderResourceView(TempIdxBuffer.Get(), &srvDesc, TempIdxSRV.GetAddressOf());

			// 메쉬별 아틀라스 맵에서 조회
			auto AtlasIt = Resources->ImpostorAtlasMap.find(TargetMesh);
			if (AtlasIt == Resources->ImpostorAtlasMap.end() || !AtlasIt->second)
			{
				continue; // 아틀라스가 없으면 이 배치는 스킵
			}
			ID3D11ShaderResourceView* TargetAtlas = AtlasIt->second.Get();

			ID3D11ShaderResourceView* VS_SRVs[] = { TargetAtlas, InRHI.InstanceSRV.Get(), TempIdxSRV.Get() };
			DeviceContext->VSSetShaderResources(0, 3, VS_SRVs);
			DeviceContext->PSSetShaderResources(0, 1, &TargetAtlas);

			DeviceContext->DrawInstanced(4, (UINT)BatchIndices.size(), 0, 0);
			DrawCallCount++;
		}

		ID3D11ShaderResourceView* NullSRVs[] = { nullptr, nullptr, nullptr };
		DeviceContext->VSSetShaderResources(0, 3, NullSRVs);
		DeviceContext->RSSetState(InRHI.GetRasterizerState(D3D11_FILL_SOLID, D3D11_CULL_BACK, FALSE));
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		DeviceContext->IASetInputLayout(Resources->InputLayout.Get());
	}
}

void FSceneRenderer::BuildHiZMipChain(const FD3D11RHI& InRHI, const FScene& InScene)
{
	ID3D11DeviceContext* DeviceContext = InRHI.GetDeviceContext();
	ID3D11ShaderResourceView* DSSRV = InRHI.GetDepthStencilSRV();

	DeviceContext->CSSetConstantBuffers(0, 1, Resources->FrameConstantBuffer.GetAddressOf());

	DeviceContext->CSSetShader(InRHI.HiZCopyDepthCS.Get(), nullptr, 0);
	DeviceContext->CSSetShaderResources(0, 1, &DSSRV);
	DeviceContext->CSSetUnorderedAccessViews(0, 1, InRHI.HiZDepthUAVs[0].GetAddressOf(), nullptr);

	DeviceContext->Dispatch(64, 64, 1);

	ID3D11ShaderResourceView* nullSRV = nullptr;
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	DeviceContext->CSSetShaderResources(0, 1, &nullSRV);
	DeviceContext->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

	DeviceContext->CSSetShader(InRHI.HiZBuildMipsCS.Get(), nullptr, 0);

	for (UINT i = 1; i <= 10; ++i)
	{
		DeviceContext->CSSetShaderResources(0, 1, InRHI.HiZDepthSRVs[i - 1].GetAddressOf());
		DeviceContext->CSSetUnorderedAccessViews(0, 1, InRHI.HiZDepthUAVs[i].GetAddressOf(), nullptr);

		UINT mipSize = std::max(1u, 1024u >> i);
		UINT dispatchCount = (UINT)std::ceil(mipSize / 16.0f);
		DeviceContext->Dispatch(dispatchCount, dispatchCount, 1);

		DeviceContext->CSSetShaderResources(0, 1, &nullSRV);
		DeviceContext->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
	}

	DeviceContext->CSSetShader(InRHI.HiZCullCS.Get(), nullptr, 0);

	ID3D11ShaderResourceView* ClearSRVs[] = { nullptr, nullptr };
	DeviceContext->VSSetShaderResources(0, 2, ClearSRVs);
	DeviceContext->PSSetShaderResources(0, 2, ClearSRVs);

	ID3D11ShaderResourceView* SRVs[] = { InRHI.InstanceSRV.Get(), InRHI.HiZFullSRV.Get(), InRHI.LastFrameVisibilitySRV.Get()};
	DeviceContext->CSSetShaderResources(0, 3, SRVs);

	DeviceContext->CSSetConstantBuffers(0, 1, Resources->FrameConstantBuffer.GetAddressOf());

	ID3D11SamplerState* samplers[] = { InRHI.PointSampler.Get() };
	DeviceContext->CSSetSamplers(0, 1, samplers);

	DeviceContext->CSSetUnorderedAccessViews(0, 1, InRHI.VisibilityUAV.GetAddressOf(), nullptr);

	const size_t PrimitiveCount = InScene.GetPrimitiveCount();
	InRHI.GetDeviceContext()->Dispatch(static_cast<UINT>(std::ceil(PrimitiveCount / 64.0f)), 1, 1);

	ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr, nullptr };
	DeviceContext->CSSetShaderResources(0, 3, nullSRVs);
	DeviceContext->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

	DeviceContext->CopyResource(InRHI.LastFrameVisibilityBuffer.Get(), InRHI.VisibilityBuffer.Get());
	DeviceContext->CopyResource(InRHI.StagingBuffer.Get(), InRHI.VisibilityBuffer.Get());
}
