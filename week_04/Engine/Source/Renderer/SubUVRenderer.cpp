#include "Renderer/SubUVRenderer.h"
#include "Renderer/Shader.h"
#include "Renderer/ShaderResource.h"
#include "Core/Paths.h"
#include <WICTextureLoader.h>
#include <cstring>
#include <algorithm>
#include "Renderer/ShaderType.h"

struct FSubUVConstantBuffer
{
	FVector4 Color;
	FVector2 CellSize;
	FVector2 Offset;
};

CSubUVRenderer::~CSubUVRenderer()
{
	Release();
}

bool CSubUVRenderer::Initialize(
	ID3D11Device* InDevice,
	ID3D11DeviceContext* InDeviceContext,
	const std::wstring& TexturePath)
{
	Release();

	Device = InDevice;
	DeviceContext = InDeviceContext;

	if (!Device || !DeviceContext)
	{
		return false;
	}

	if (!CreateShaders())
	{
		return false;
	}

	if (!CreateConstantBuffers())
	{
		return false;
	}

	if (!CreateRenderStates())
	{
		return false;
	}

	if (!CreateTextureAndSampler(TexturePath))
	{
		return false;
	}

	return true;
}

void CSubUVRenderer::Release()
{
	SubUVVS.reset();
	SubUVPS.reset();

	if (TextureSRV)
	{
		TextureSRV->Release();
		TextureSRV = nullptr;
	}

	if (SamplerState)
	{
		SamplerState->Release();
		SamplerState = nullptr;
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

	if (SubUVConstantBuffer)
	{
		SubUVConstantBuffer->Release();
		SubUVConstantBuffer = nullptr;
	}

	if (DynamicVertexBuffer)
	{
		DynamicVertexBuffer->Release();
		DynamicVertexBuffer = nullptr;
	}

	if (DynamicIndexBuffer)
	{
		DynamicIndexBuffer->Release();
		DynamicIndexBuffer = nullptr;
	}

	if (AlphaBlendState)
	{
		AlphaBlendState->Release();
		AlphaBlendState = nullptr;
	}

	if (NoCullRasterizerState)
	{
		NoCullRasterizerState->Release();
		NoCullRasterizerState = nullptr;
	}

	DynamicVertexCapacity = 0;
	DynamicIndexCapacity = 0;

	Device = nullptr;
	DeviceContext = nullptr;
}

bool CSubUVRenderer::CreateShaders()
{
	const std::filesystem::path ShaderDir = FPaths::ShaderDir();
	const std::wstring VSPath = (ShaderDir / "SubUVVertexShader.hlsl").wstring();
	const std::wstring PSPath = (ShaderDir / "SubUVPixelShader.hlsl").wstring();


	auto VSResource = FShaderResource::GetOrCompile(VSPath.c_str(), "main", "vs_5_0");
	if (!VSResource)
	{
		return false;
	}

	auto PSResource = FShaderResource::GetOrCompile(PSPath.c_str(), "main", "ps_5_0");
	if (!PSResource)
	{
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC Layout[] =
	{
	  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	  { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	SubUVVS = FVertexShader::CreateWithLayout(
		Device,
		VSResource,
		Layout,
		ARRAYSIZE(Layout)
	);

	if (!SubUVVS)
	{
		return false;
	}

	SubUVPS = FPixelShader::Create(Device, PSResource);
	if (!SubUVPS)
	{
		return false;
	}

	return true;
}

bool CSubUVRenderer::CreateConstantBuffers()
{
	D3D11_BUFFER_DESC Desc = {};
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	Desc.ByteWidth = sizeof(FFrameConstantBuffer);
	if (FAILED(Device->CreateBuffer(&Desc, nullptr, &FrameConstantBuffer)))
	{
		return false;
	}

	Desc.ByteWidth = sizeof(FObjectConstantBuffer);
	if (FAILED(Device->CreateBuffer(&Desc, nullptr, &ObjectConstantBuffer)))
	{
		return false;
	}

	Desc.ByteWidth = sizeof(FSubUVConstantBuffer);
	if (FAILED(Device->CreateBuffer(&Desc, nullptr, &SubUVConstantBuffer)))
	{
		return false;
	}

	return true;
}

bool CSubUVRenderer::CreateRenderStates()
{
	D3D11_BLEND_DESC BlendDesc = {};
	BlendDesc.RenderTarget[0].BlendEnable = TRUE;
	BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	if (FAILED(Device->CreateBlendState(&BlendDesc, &AlphaBlendState)))
	{
		return false;
	}

	D3D11_RASTERIZER_DESC RasterDesc = {};
	RasterDesc.FillMode = D3D11_FILL_SOLID;
	RasterDesc.CullMode = D3D11_CULL_NONE;
	RasterDesc.DepthClipEnable = TRUE;

	if (FAILED(Device->CreateRasterizerState(&RasterDesc, &NoCullRasterizerState)))
	{
		return false;
	}

	return true;
}

bool CSubUVRenderer::CreateTextureAndSampler(const std::wstring& TexturePath)
{
	HRESULT Hr = DirectX::CreateWICTextureFromFile(
		Device,
		DeviceContext,
		TexturePath.c_str(),
		nullptr,
		&TextureSRV
	);

	if (FAILED(Hr))
	{
		return false;
	}

	D3D11_SAMPLER_DESC SamplerDesc = {};
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	if (FAILED(Device->CreateSamplerState(&SamplerDesc, &SamplerState)))
	{
		return false;
	}

	return true;
}

void CSubUVRenderer::Begin(
	const FMatrix& InView,
	const FMatrix& InProjection,
	const FVector& InCameraPosition)
{
	ViewMatrix = InView;
	ProjectionMatrix = InProjection;
	CameraPosition = InCameraPosition;

	UpdateFrameCB();
}

void CSubUVRenderer::EnsureDynamicBuffers(uint32 VertexCount, uint32 IndexCount)
{
	if (VertexCount > DynamicVertexCapacity)
	{
		if (DynamicVertexBuffer)
		{
			DynamicVertexBuffer->Release();
			DynamicVertexBuffer = nullptr;
		}

		D3D11_BUFFER_DESC Desc = {};
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Desc.ByteWidth = sizeof(FTextureVertex) * VertexCount;

		if (SUCCEEDED(Device->CreateBuffer(&Desc, nullptr, &DynamicVertexBuffer)))
		{
			DynamicVertexCapacity = VertexCount;
		}
	}

	if (IndexCount > DynamicIndexCapacity)
	{
		if (DynamicIndexBuffer)
		{
			DynamicIndexBuffer->Release();
			DynamicIndexBuffer = nullptr;
		}

		D3D11_BUFFER_DESC Desc = {};
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Desc.ByteWidth = sizeof(uint32) * IndexCount;

		if (SUCCEEDED(Device->CreateBuffer(&Desc, nullptr, &DynamicIndexBuffer)))
		{
			DynamicIndexCapacity = IndexCount;
		}
	}
}

void CSubUVRenderer::UpdateFrameCB()
{
	FFrameConstantBuffer CBData;
	CBData.View = ViewMatrix.GetTransposed();
	CBData.Projection = ProjectionMatrix.GetTransposed();

	D3D11_MAPPED_SUBRESOURCE Mapped = {};
	if (SUCCEEDED(DeviceContext->Map(FrameConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
	{
		std::memcpy(Mapped.pData, &CBData, sizeof(CBData));
		DeviceContext->Unmap(FrameConstantBuffer, 0);
	}
}

void CSubUVRenderer::UpdateObjectCB(const FMatrix& WorldMatrix)
{
	FObjectConstantBuffer CBData;
	CBData.World = WorldMatrix.GetTransposed();

	D3D11_MAPPED_SUBRESOURCE Mapped = {};
	if (SUCCEEDED(DeviceContext->Map(ObjectConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
	{
		std::memcpy(Mapped.pData, &CBData, sizeof(CBData));
		DeviceContext->Unmap(ObjectConstantBuffer, 0);
	}
}

void CSubUVRenderer::UpdateSubUVCB(
	const FVector4& Color,
	const FVector2& CellSize,
	const FVector2& Offset)
{
	FSubUVConstantBuffer CBData;
	CBData.Color = Color;
	CBData.CellSize = CellSize;
	CBData.Offset = Offset;

	D3D11_MAPPED_SUBRESOURCE Mapped = {};
	if (SUCCEEDED(DeviceContext->Map(SubUVConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
	{
		std::memcpy(Mapped.pData, &CBData, sizeof(CBData));
		DeviceContext->Unmap(SubUVConstantBuffer, 0);
	}
}

void CSubUVRenderer::DrawSubUV(
	const FVector& WorldPosition,
	const FVector2& Size,
	const FVector4& Color,
	int32 Columns,
	int32 Rows,
	int32 TotalFrames,
	int32 FirstFrame,
	int32 LastFrame,
	float FPS,
	float ElapsedTime,
	bool bLoop,
	bool bBillboard)
{
	if (!Device || !DeviceContext || !SubUVVS || !SubUVPS || !TextureSRV)
	{
		return;
	}

	if (Columns <= 0 || Rows <= 0 || TotalFrames <= 0 || FPS <= 0.0f)
	{
		return;
	}

	float FrameFloat = ElapsedTime * FPS;
	int32 AnimationFrame = static_cast<int32>(FrameFloat);

	FirstFrame = std::max<int32>(0, std::min<int32>(FirstFrame, TotalFrames - 1));
	LastFrame = std::max<int32>(0, std::min<int32>(LastFrame, TotalFrames - 1));

	if (FirstFrame > LastFrame)
	{
		std::swap(FirstFrame, LastFrame);
	}

	const int32 PlayableFrameCount = LastFrame - FirstFrame + 1;

	int32 FrameIndex = FirstFrame;

	if (PlayableFrameCount > 0)
	{
		if (bLoop)
		{
			FrameIndex = FirstFrame + (AnimationFrame % PlayableFrameCount);
		}
		else
		{
			FrameIndex = FirstFrame + std::min<int32>(AnimationFrame, PlayableFrameCount - 1);
		}
	}

	const int32 Col = FrameIndex % Columns;
	const int32 Row = FrameIndex / Columns;

	const float CellWidth = 1.0f / static_cast<float>(Columns);
	const float CellHeight = 1.0f / static_cast<float>(Rows);

	FVector2 CellSize(CellWidth, CellHeight);

	// 텍스처가 위->아래 순으로 정렬된 atlas 기준
	FVector2 UVOffset(
		static_cast<float>(Col) * CellWidth,
		static_cast<float>(Row) * CellHeight
	);

	TArray<FTextureVertex> Vertices;
	TArray<uint32> Indices;

	Vertices.reserve(4);
	Indices.reserve(6);

	const float HalfW = Size.X * 0.5f;
	const float HalfH = Size.Y * 0.5f;

	Vertices.push_back(FTextureVertex(FVector(0.0f, -HalfW, HalfH), FVector2(0.0f, 0.0f)));
	Vertices.push_back(FTextureVertex(FVector(0.0f, HalfW, HalfH), FVector2(1.0f, 0.0f)));
	Vertices.push_back(FTextureVertex(FVector(0.0f, HalfW, -HalfH), FVector2(1.0f, 1.0f)));
	Vertices.push_back(FTextureVertex(FVector(0.0f, -HalfW, -HalfH), FVector2(0.0f, 1.0f)));

	Indices.push_back(0);
	Indices.push_back(1);
	Indices.push_back(2);
	Indices.push_back(0);
	Indices.push_back(2);
	Indices.push_back(3);

	EnsureDynamicBuffers(4, 6);

	if (!DynamicVertexBuffer || !DynamicIndexBuffer)
	{
		return;
	}

	D3D11_MAPPED_SUBRESOURCE Mapped = {};

	if (SUCCEEDED(DeviceContext->Map(DynamicVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
	{
		std::memcpy(Mapped.pData, Vertices.data(), sizeof(FTextureVertex) * Vertices.size());
		DeviceContext->Unmap(DynamicVertexBuffer, 0);
	}

	if (SUCCEEDED(DeviceContext->Map(DynamicIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
	{
		std::memcpy(Mapped.pData, Indices.data(), sizeof(uint32) * Indices.size());
		DeviceContext->Unmap(DynamicIndexBuffer, 0);
	}

	FMatrix World = FMatrix::Identity;

	if (bBillboard)
	{
		World = FMatrix::MakeBillboard(WorldPosition, CameraPosition);
	}
	else
	{
		World = FMatrix::MakeTranslation(WorldPosition);
	}

	UpdateObjectCB(World);
	UpdateSubUVCB(Color, CellSize, UVOffset);

	SubUVVS->Bind(DeviceContext);
	SubUVPS->Bind(DeviceContext);

	ID3D11Buffer* VSConstantBuffers[2] = { FrameConstantBuffer, ObjectConstantBuffer };
	DeviceContext->VSSetConstantBuffers(0, 2, VSConstantBuffers);
	DeviceContext->PSSetConstantBuffers(2, 1, &SubUVConstantBuffer);

	DeviceContext->PSSetShaderResources(0, 1, &TextureSRV);
	DeviceContext->PSSetSamplers(0, 1, &SamplerState);

	const float BlendFactor[4] = { 0, 0, 0, 0 };
	DeviceContext->OMSetBlendState(AlphaBlendState, BlendFactor, 0xffffffff);
	DeviceContext->RSSetState(NoCullRasterizerState);

	UINT Stride = sizeof(FTextureVertex);
	UINT Offset = 0;

	DeviceContext->IASetVertexBuffers(0, 1, &DynamicVertexBuffer, &Stride, &Offset);
	DeviceContext->IASetIndexBuffer(DynamicIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	DeviceContext->DrawIndexed(6, 0, 0);

	ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
	DeviceContext->PSSetShaderResources(0, 1, NullSRV);

	DeviceContext->OMSetBlendState(nullptr, BlendFactor, 0xffffffff);
	DeviceContext->RSSetState(nullptr);
}