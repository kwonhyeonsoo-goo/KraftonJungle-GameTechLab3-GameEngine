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
};

struct alignas(16) FObjectConstants
{
	FMatrix World = FMatrix::Identity;
	float Tint[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct FSceneRenderer::FResources
{
	TComPtr<ID3D11VertexShader> VertexShader;
	TComPtr<ID3D11PixelShader> PixelShader;
	TComPtr<ID3D11InputLayout> InputLayout;
	TComPtr<ID3D11Buffer> FrameConstantBuffer;
	TComPtr<ID3D11Buffer> ObjectConstantBuffer;
	TComPtr<ID3D11SamplerState> LinearSampler;
	TComPtr<ID3D11RasterizerState> RasterizerState;
	TComPtr<ID3D11DepthStencilState> DepthStencilState;
	TComPtr<ID3D11BlendState> BlendState;
	TComPtr<ID3D11ShaderResourceView> WhiteTextureView;
};

FSceneRenderer::FSceneRenderer() = default;
FSceneRenderer::~FSceneRenderer() = default;

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
};

cbuffer ObjectCB : register(b1)
{
    row_major float4x4 World;
    float4 Tint;
};

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
    float4 WorldPosition = mul(float4(Input.Position, 1.0f), World);
    Output.Position = mul(WorldPosition, ViewProjection);
    Output.TexCoord = Input.TexCoord;
    return Output;
}
)";

	static constexpr char PixelShaderSource[] = R"(
cbuffer ObjectCB : register(b1)
{
    row_major float4x4 World;
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

	if (!D3D11Utils::CreateDynamicConstantBuffer(Device, sizeof(FFrameConstants), Resources->FrameConstantBuffer)
		|| !D3D11Utils::CreateDynamicConstantBuffer(Device, sizeof(FObjectConstants), Resources->ObjectConstantBuffer))
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
		D3D11_CULL_NONE,
		FALSE,
		0,
		0.0f,
		0.0f,
		TRUE,
		FALSE,
		FALSE,
		FALSE
	};

	if (FAILED(Device->CreateRasterizerState(&RasterizerDesc, Resources->RasterizerState.GetAddressOf())))
	{
		Resources.reset();
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC DepthStencilDesc = {};
	DepthStencilDesc.DepthEnable = TRUE;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	DepthStencilDesc.StencilEnable = FALSE;

	if (FAILED(Device->CreateDepthStencilState(&DepthStencilDesc, Resources->DepthStencilState.GetAddressOf())))
	{
		Resources.reset();
		return false;
	}

	D3D11_BLEND_DESC BlendDesc = {};
	BlendDesc.RenderTarget[0].BlendEnable = FALSE;
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	if (FAILED(Device->CreateBlendState(&BlendDesc, Resources->BlendState.GetAddressOf())))
	{
		Resources.reset();
		return false;
	}

	if (!CreateWhiteTexture(Device, Resources->WhiteTextureView))
	{
		Resources.reset();
		return false;
	}

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
	if (!Resources)
	{
		return;
	}

	ID3D11DeviceContext* DeviceContext = InRHI.GetDeviceContext();
	if (DeviceContext == nullptr)
	{
		return;
	}

	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	HRESULT hr = InRHI.GetDeviceContext()->Map(InRHI.StagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &MappedResource);

	uint32* LastFrameVisibility = nullptr;
	if (SUCCEEDED(hr))
	{
		LastFrameVisibility = static_cast<uint32*>(MappedResource.pData);
	}

	ID3D11RenderTargetView* RenderTargets[] = { InRHI.GetBackBufferRTV() };
	const D3D11_VIEWPORT Viewport = InRHI.GetViewport();
	DeviceContext->OMSetRenderTargets(1, RenderTargets, InRHI.GetDepthStencilView());
	DeviceContext->RSSetViewports(1, &Viewport);

	DeviceContext->IASetInputLayout(Resources->InputLayout.Get());
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DeviceContext->VSSetShader(Resources->VertexShader.Get(), nullptr, 0);
	DeviceContext->PSSetShader(Resources->PixelShader.Get(), nullptr, 0);
	DeviceContext->RSSetState(Resources->RasterizerState.Get());
	DeviceContext->OMSetDepthStencilState(Resources->DepthStencilState.Get(), 0);
	DeviceContext->OMSetBlendState(Resources->BlendState.Get(), nullptr, 0xffffffffu);

	ID3D11SamplerState* Samplers[] = { Resources->LinearSampler.Get() };
	DeviceContext->PSSetSamplers(0, 1, Samplers);

	FFrameConstants FrameConstants = {};
	FrameConstants.ViewProjection = InCamera.GetViewMatrix() * InCamera.GetProjectionMatrix();
	FrameConstants.RenderTargetSize = { static_cast<float>(InRHI.GetViewportWidth()), static_cast<float>(InRHI.GetViewportHeight()) };
	D3D11Utils::UpdateDynamicBuffer(DeviceContext, Resources->FrameConstantBuffer.Get(), FrameConstants);

	ID3D11Buffer* VertexConstantBuffers[] = { Resources->FrameConstantBuffer.Get(), Resources->ObjectConstantBuffer.Get() };
	ID3D11Buffer* PixelConstantBuffers[] = { nullptr, Resources->ObjectConstantBuffer.Get() };
	DeviceContext->VSSetConstantBuffers(0, 2, VertexConstantBuffers);
	DeviceContext->PSSetConstantBuffers(0, 2, PixelConstantBuffers);

	ID3D11Buffer* CurrentVertexBuffer = nullptr;
	ID3D11Buffer* CurrentIndexBuffer = nullptr;
	ID3D11ShaderResourceView* CurrentTextureView = nullptr;
	const UINT Stride = sizeof(FStaticMeshVertex);
	const UINT Offset = 0;

	const TArray<FScenePrimitiveRuntimeData>& PrimitiveRuntimeData = InScene.GetPrimitiveRuntimeData();
	for (uint32 PrimitiveIndex : InVisibilityResults.VisiblePrimitiveIndices)
	{
		if (PrimitiveIndex >= PrimitiveRuntimeData.size())
		{
			continue;
		}
		if (LastFrameVisibility)
		{
			uint32 IsVisible = LastFrameVisibility[PrimitiveIndex];

			if (IsVisible == 0)
			{
				continue;
			}
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

		FObjectConstants ObjectConstants = {};
		ObjectConstants.World = PrimitiveData.WorldMatrix;
		if (PrimitiveData.PrimitiveId == InPickState.SelectedPrimitiveId)
		{
			ObjectConstants.Tint[0] = 0.1f;
			ObjectConstants.Tint[1] = 0.1f;
			ObjectConstants.Tint[2] = 0.1f;
			ObjectConstants.Tint[3] = 1.0f;
		}

		D3D11Utils::UpdateDynamicBuffer(DeviceContext, Resources->ObjectConstantBuffer.Get(), ObjectConstants);

		for (const FStaticMesh::FSection& Section : StaticMesh->GetSections())
		{
			ID3D11ShaderResourceView* TextureView = StaticMesh->GetMaterialTexture(Section.MaterialIndex);
			if (TextureView == nullptr)
			{
				TextureView = Resources->WhiteTextureView.Get();
			}

			if (TextureView != CurrentTextureView)
			{
				DeviceContext->PSSetShaderResources(0, 1, &TextureView);
				CurrentTextureView = TextureView;
			}

			DeviceContext->DrawIndexed(Section.IndexCount, Section.IndexStart, 0);
		}
	}

	if (LastFrameVisibility)
	{
		InRHI.GetDeviceContext()->Unmap(InRHI.StagingBuffer.Get(), 0);
	}

	ID3D11RenderTargetView* NullRTVs[] = { nullptr };
	DeviceContext->OMSetRenderTargets(1, NullRTVs, nullptr);

	ID3D11ShaderResourceView* DSSRV = InRHI.GetDepthStencilSRV();

	InRHI.GetDeviceContext()->CSSetShader(InRHI.HiZCopyDepthCS.Get(), nullptr, 0);
	InRHI.GetDeviceContext()->CSSetShaderResources(0, 1, &DSSRV);
	InRHI.GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, InRHI.HiZDepthUAVs[0].GetAddressOf(), nullptr);

	InRHI.GetDeviceContext()->Dispatch(64, 64, 1);

	ID3D11ShaderResourceView* nullSRV = nullptr;
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	InRHI.GetDeviceContext()->CSSetShaderResources(0, 1, &nullSRV);
	InRHI.GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

	InRHI.GetDeviceContext()->CSSetShader(InRHI.HiZBuildMipsCS.Get(), nullptr, 0);

	for (UINT i = 1; i <= 10; ++i)
	{
		InRHI.GetDeviceContext()->CSSetShaderResources(0, 1, InRHI.HiZDepthSRVs[i - 1].GetAddressOf());
		InRHI.GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, InRHI.HiZDepthUAVs[i].GetAddressOf(), nullptr);

		UINT mipSize = std::max(1u, 1024u >> i);
		UINT dispatchCount = (UINT)std::ceil(mipSize / 16.0f);
		InRHI.GetDeviceContext()->Dispatch(dispatchCount, dispatchCount, 1);

		InRHI.GetDeviceContext()->CSSetShaderResources(0, 1, &nullSRV);
		InRHI.GetDeviceContext()->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
	}

	InRHI.GetDeviceContext()->CSSetShader(InRHI.HiZCullCS.Get(), nullptr, 0);

	ID3D11ShaderResourceView* SRVs[] = { InRHI.InstanceSRV.Get(), InRHI.HiZFullSRV.Get() };
	DeviceContext->CSSetShaderResources(0, 2, SRVs);

	DeviceContext->CSSetConstantBuffers(0, 1, Resources->FrameConstantBuffer.GetAddressOf()); //

	ID3D11SamplerState* samplers[] = { InRHI.PointSampler.Get() };
	DeviceContext->CSSetSamplers(0, 1, samplers);

	DeviceContext->CSSetUnorderedAccessViews(0, 1, InRHI.VisibilityUAV.GetAddressOf(), nullptr);

	InRHI.GetDeviceContext()->Dispatch(static_cast<UINT>(std::ceil(50000 / 64.0f)), 1, 1);

	ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr };
	DeviceContext->CSSetShaderResources(0, 2, nullSRVs);
	DeviceContext->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

	InRHI.GetDeviceContext()->CopyResource(InRHI.StagingBuffer.Get(), InRHI.VisibilityBuffer.Get());
}
