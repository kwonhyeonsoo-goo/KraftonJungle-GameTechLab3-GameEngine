#include "Renderer/SceneRenderer.h"

#include <cstddef>
#include <cstring>

#include <d3dcompiler.h>

#include "Camera/Camera.h"
#include "Graphics/D3D11/D3D11RHI.h"
#include "Picking/PickingSystem.h"
#include "Scene/Scene.h"
#include "StaticMesh/StaticMesh.h"
#include "Visibility/VisibilitySystem.h"

namespace
{
	bool CreateDynamicConstantBuffer(ID3D11Device* InDevice, UINT InByteWidth, TComPtr<ID3D11Buffer>& OutBuffer)
	{
		const D3D11_BUFFER_DESC BufferDesc =
		{
			InByteWidth,
			D3D11_USAGE_DYNAMIC,
			D3D11_BIND_CONSTANT_BUFFER,
			D3D11_CPU_ACCESS_WRITE,
			0,
			0
		};

		return SUCCEEDED(InDevice->CreateBuffer(&BufferDesc, nullptr, OutBuffer.GetAddressOf()));
	}

	template <typename T>
	bool UpdateDynamicBuffer(ID3D11DeviceContext* InDeviceContext, ID3D11Buffer* InBuffer, const T& InData)
	{
		if (InDeviceContext == nullptr || InBuffer == nullptr)
		{
			return false;
		}

		D3D11_MAPPED_SUBRESOURCE MappedResource = {};
		if (FAILED(InDeviceContext->Map(InBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
		{
			return false;
		}

		std::memcpy(MappedResource.pData, &InData, sizeof(T));
		InDeviceContext->Unmap(InBuffer, 0);
		return true;
	}

	bool CompileShader(const char* InSource, const char* InEntryPoint, const char* InTarget, TComPtr<ID3DBlob>& OutBlob)
	{
		UINT CompileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
		CompileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		TComPtr<ID3DBlob> ErrorBlob;
		const HRESULT Result = D3DCompile(
			InSource,
			std::strlen(InSource),
			nullptr,
			nullptr,
			nullptr,
			InEntryPoint,
			InTarget,
			CompileFlags,
			0,
			OutBlob.GetAddressOf(),
			ErrorBlob.GetAddressOf());

		return SUCCEEDED(Result);
	}

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
	if (!CompileShader(VertexShaderSource, "VSMain", "vs_5_0", VertexShaderBlob)
		|| !CompileShader(PixelShaderSource, "PSMain", "ps_5_0", PixelShaderBlob))
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

	if (!CreateDynamicConstantBuffer(Device, sizeof(FFrameConstants), Resources->FrameConstantBuffer)
		|| !CreateDynamicConstantBuffer(Device, sizeof(FObjectConstants), Resources->ObjectConstantBuffer))
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

	DeviceContext->IASetInputLayout(Resources->InputLayout.Get());
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DeviceContext->VSSetShader(Resources->VertexShader.Get(), nullptr, 0);
	DeviceContext->PSSetShader(Resources->PixelShader.Get(), nullptr, 0);
	DeviceContext->RSSetState(Resources->RasterizerState.Get());

	ID3D11SamplerState* Samplers[] = { Resources->LinearSampler.Get() };
	DeviceContext->PSSetSamplers(0, 1, Samplers);

	FFrameConstants FrameConstants = {};
	FrameConstants.ViewProjection = InCamera.GetViewMatrix() * InCamera.GetProjectionMatrix();
	UpdateDynamicBuffer(DeviceContext, Resources->FrameConstantBuffer.Get(), FrameConstants);

	ID3D11Buffer* VertexConstantBuffers[] = { Resources->FrameConstantBuffer.Get(), Resources->ObjectConstantBuffer.Get() };
	ID3D11Buffer* PixelConstantBuffers[] = { nullptr, Resources->ObjectConstantBuffer.Get() };
	DeviceContext->VSSetConstantBuffers(0, 2, VertexConstantBuffers);
	DeviceContext->PSSetConstantBuffers(0, 2, PixelConstantBuffers);

	ID3D11Buffer* CurrentVertexBuffer = nullptr;
	ID3D11ShaderResourceView* CurrentTextureView = nullptr;
	const UINT Stride = sizeof(FStaticMeshVertex);
	const UINT Offset = 0;

	const TArray<FRenderItem>& RenderItems = InScene.GetRenderItems();
	for (uint32 PrimitiveIndex : InVisibilityResults.VisiblePrimitiveIndices)
	{
		if (PrimitiveIndex >= RenderItems.size())
		{
			continue;
		}

		const FRenderItem& RenderItem = RenderItems[PrimitiveIndex];
		if (!RenderItem.StaticMesh || !RenderItem.StaticMesh->IsValid())
		{
			continue;
		}

		ID3D11Buffer* VertexBuffer = RenderItem.StaticMesh->GetVertexBuffer();
		if (VertexBuffer != CurrentVertexBuffer)
		{
			DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
			CurrentVertexBuffer = VertexBuffer;
		}

		ID3D11ShaderResourceView* TextureView = RenderItem.StaticMesh->GetDiffuseTexture();
		if (TextureView == nullptr)
		{
			TextureView = Resources->WhiteTextureView.Get();
		}

		if (TextureView != CurrentTextureView)
		{
			DeviceContext->PSSetShaderResources(0, 1, &TextureView);
			CurrentTextureView = TextureView;
		}

		FObjectConstants ObjectConstants = {};
		ObjectConstants.World = RenderItem.Transform.ToMatrixWithScale();
		if (RenderItem.PrimitiveId == InPickState.SelectedPrimitiveId)
		{
			ObjectConstants.Tint[0] = 1.0f;
			ObjectConstants.Tint[1] = 0.6f;
			ObjectConstants.Tint[2] = 0.25f;
			ObjectConstants.Tint[3] = 1.0f;
		}

		UpdateDynamicBuffer(DeviceContext, Resources->ObjectConstantBuffer.Get(), ObjectConstants);
		DeviceContext->Draw(RenderItem.StaticMesh->GetVertexCount(), 0);
	}
}
