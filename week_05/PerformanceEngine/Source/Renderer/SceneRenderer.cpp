#include "Renderer/SceneRenderer.h"

#include <cstddef>

#include "Camera/Camera.h"
#include "Graphics/D3D11/D3D11Utils.h"
#include "Graphics/D3D11/D3D11RHI.h"
#include "Picking/PickingSystem.h"
#include "Scene/Scene.h"
#include "StaticMesh/StaticMesh.h"
#include "Visibility/VisibilitySystem.h"

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
	D3D11Utils::UpdateDynamicBuffer(DeviceContext, Resources->FrameConstantBuffer.Get(), FrameConstants);

	//상수 버퍼 세팅 (b0: Frame, b1: InstanceIndex, b2: Highlight)
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

	ID3D11Buffer* CurrentVertexBuffer = nullptr;
	ID3D11Buffer* CurrentIndexBuffer = nullptr;
	ID3D11ShaderResourceView* CurrentTextureView = nullptr;
	const UINT Stride = sizeof(FStaticMeshVertex);
	const UINT Offset = 0;

	const TArray<FScenePrimitiveRuntimeData>& PrimitiveRuntimeData = InScene.GetPrimitiveRuntimeData();

	for (uint32 PrimitiveIndex : VisibleIndices)
	{
		if (PrimitiveIndex >= PrimitiveRuntimeData.size())
		{
			continue;
		}

		const FScenePrimitiveRuntimeData& PrimitiveData = PrimitiveRuntimeData[PrimitiveIndex];
		FStaticMesh* StaticMesh = PrimitiveData.StaticMesh;
		if (StaticMesh == nullptr || !StaticMesh->IsValid())
		{
			continue;
		}


		ID3D11Buffer* VertexBuffer = StaticMesh->GetVertexBuffer();
		if (VertexBuffer != CurrentVertexBuffer)
		{
			DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
			CurrentVertexBuffer = VertexBuffer;
		}

		ID3D11Buffer* IndexBuffer = StaticMesh->GetIndexBuffer();
		if (IndexBuffer != CurrentIndexBuffer)
		{
			DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
			CurrentIndexBuffer = IndexBuffer;
		}

		FInstanceIndexConstants IndexData;
		IndexData.InstanceIndex = PrimitiveIndex;
		if (PrimitiveData.PrimitiveId == InPickState.SelectedPrimitiveId)
		{
			IndexData.Tint[0] = 1.0f;
			IndexData.Tint[1] = 0.2f;
			IndexData.Tint[2] = 0.2f;
			IndexData.Tint[3] = 1.0f;
		}
		else
		{
			IndexData.Tint[0] = 1.0f;
			IndexData.Tint[1] = 1.0f;
			IndexData.Tint[2] = 1.0f;
			IndexData.Tint[3] = 1.0f;
		}
		D3D11Utils::UpdateDynamicBuffer(DeviceContext, Resources->InstanceIndexConstantBuffer.Get(), IndexData);
		for (const FStaticMesh::FSection& Section : StaticMesh->GetSections())
		{
			ID3D11ShaderResourceView* TextureView = StaticMesh->GetMaterialTexture(Section.MaterialIndex);
			if (TextureView == nullptr) TextureView = Resources->WhiteTextureView.Get();

			if (TextureView != CurrentTextureView)
			{
				DeviceContext->PSSetShaderResources(0, 1, &TextureView); // t0에 텍스처 세팅
				CurrentTextureView = TextureView;
			}
			DeviceContext->DrawIndexed(Section.IndexCount, Section.IndexStart, 0);

			++DrawCallCount;
		}
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
	DeviceContext->Dispatch(static_cast<UINT>(std::ceil(PrimitiveCount / 64.0f)), 1, 1);

	ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr, nullptr };
	DeviceContext->CSSetShaderResources(0, 3, nullSRVs);
	DeviceContext->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

	DeviceContext->CopyResource(InRHI.LastFrameVisibilityBuffer.Get(), InRHI.VisibilityBuffer.Get());
	DeviceContext->CopyResource(InRHI.StagingBuffer.Get(), InRHI.VisibilityBuffer.Get());
}
