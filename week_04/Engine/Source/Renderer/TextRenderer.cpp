#include "Renderer/TextRenderer.h"
#include "Renderer/Shader.h"
#include "Renderer/ShaderResource.h"
#include "Renderer/ShaderType.h"
#include "Core/Paths.h"
#include <cstring>

struct FTextConstantBuffer
{
	FVector4 TextColor;
};

CTextRenderer::~CTextRenderer()
{
	Release();
}

bool CTextRenderer::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext)
{
	Release();

	Device = InDevice;
	DeviceContext = InDeviceContext;

	if (!Device || !DeviceContext)
	{
		MessageBox(0, L"TextRenderer: Device or DeviceContext is null", 0, 0);
		return false;
	}

	if (!CreateShaders())
	{
		MessageBox(0, L"TextRenderer: CreateShaders failed", 0, 0);
		return false;
	}

	if (!CreateConstantBuffers())
	{
		MessageBox(0, L"TextRenderer: CreateConstantBuffers failed", 0, 0);
		return false;
	}

	if (!CreateRenderStates())
	{
		MessageBox(0, L"TextRenderer: CreateRenderStates failed", 0, 0);
		return false;
	}

	const std::wstring FontPath = FPaths::ToWide(FPaths::ContentDir() + "Fonts/NotoSansKR_Atlas.png");
	//const std::wstring FontPath = (L"C:\\Users\\Seyoung\\source\\repos\\Week3_Team5_DinoEngine\\Content\\Fonts\\NotoSansKR_Atlas.png");
	if (!Atlas.Initialize(Device, DeviceContext, FontPath))
	{
		MessageBox(0, L"TextRenderer: Atlas.Initialize failed", 0, 0);
		return false;
	}

	return true;
}

void CTextRenderer::Release()
{
	Atlas.Release();

	FontVS.reset();
	FontPS.reset();

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

	if (TextConstantBuffer)
	{
		TextConstantBuffer->Release();
		TextConstantBuffer = nullptr;
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

bool CTextRenderer::CreateShaders()
{
	const std::wstring ShaderDir = FPaths::ToWide(FPaths::ShaderDir());
	const std::wstring VSPath = ShaderDir + L"FontVertexShader.hlsl";
	const std::wstring PSPath = ShaderDir + L"FontPixelShader.hlsl";

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

	D3D11_INPUT_ELEMENT_DESC FontLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	FontVS = FVertexShader::CreateWithLayout(
		Device,
		VSResource,
		FontLayout,
		ARRAYSIZE(FontLayout)
	);

	if (!FontVS)
	{
		return false;
	}

	FontPS = FPixelShader::Create(Device, PSResource);
	if (!FontPS)
	{
		return false;
	}

	return true;
}

bool CTextRenderer::CreateConstantBuffers()
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

	Desc.ByteWidth = sizeof(FTextConstantBuffer);
	if (FAILED(Device->CreateBuffer(&Desc, nullptr, &TextConstantBuffer)))
	{
		return false;
	}

	return true;
}

bool CTextRenderer::CreateRenderStates()
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
	RasterDesc.FrontCounterClockwise = FALSE;
	RasterDesc.DepthClipEnable = TRUE;

	if (FAILED(Device->CreateRasterizerState(&RasterDesc, &NoCullRasterizerState)))
	{
		return false;
	}

	return true;
}

void CTextRenderer::Begin(const FMatrix& InView, const FMatrix& InProjection, const FVector& InCameraPosition)
{
	ViewMatrix = InView;
	ProjectionMatrix = InProjection;
	CameraPosition = InCameraPosition;

	UpdateFrameCB();
}

void CTextRenderer::EnsureDynamicBuffers(uint32 VertexCount, uint32 IndexCount)
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
		Desc.ByteWidth = sizeof(FFontVertex) * VertexCount;

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

void CTextRenderer::UpdateFrameCB()
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

void CTextRenderer::UpdateObjectCB(const FMatrix& WorldMatrix)
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

void CTextRenderer::UpdateTextCB(const FVector4& Color)
{
	FTextConstantBuffer CBData;
	CBData.TextColor = Color;

	D3D11_MAPPED_SUBRESOURCE Mapped = {};
	if (SUCCEEDED(DeviceContext->Map(TextConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
	{
		std::memcpy(Mapped.pData, &CBData, sizeof(CBData));
		DeviceContext->Unmap(TextConstantBuffer, 0);
	}
}

TArray<uint32> CTextRenderer::DecodeToCodepoints(const FString& Text) const
{
	TArray<uint32> Result;
	Result.reserve(Text.size());

	// 현재 단계:
	// std::string(FString) 내부의 각 바이트를 그대로 codepoint로 사용
	// 한글 UTF-8 완전 지원은 이후 여기만 교체
	for (unsigned char Byte : Text)
	{
		Result.push_back(static_cast<uint32>(Byte));
	}

	return Result;
}

void CTextRenderer::DrawTextBillboard(
	const FString& Text,
	const FVector& WorldPosition,
	float WorldScale,
	const FVector4& Color)
{
	if (!Device || !DeviceContext || !FontVS || !FontPS || Text.empty())
	{
		return;
	}

	const TArray<uint32> Codepoints = DecodeToCodepoints(Text);
	if (Codepoints.empty())
	{
		return;
	}

	float TotalWidth = 0.0f;
	for (uint32 Cp : Codepoints)
	{
		const FFontGlyph& Glyph = Atlas.GetGlyph(Cp);
		TotalWidth += Glyph.Advance;
	}

	float PenX = -TotalWidth * 0.5f;

	TArray<FFontVertex> Vertices;
	TArray<uint32> Indices;

	Vertices.reserve(Codepoints.size() * 4);
	Indices.reserve(Codepoints.size() * 6);

	for (uint32 Cp : Codepoints)
	{
		const FFontGlyph& Glyph = Atlas.GetGlyph(Cp);

		if (Glyph.Width > 0.0f && Glyph.Height > 0.0f)
		{
			const float X0 = PenX;
			const float X1 = PenX + Glyph.Width;
			const float Y0 = 0.0f;
			const float Y1 = Glyph.Height;

			const uint32 BaseIndex = static_cast<uint32>(Vertices.size());

			// 로컬 공간에서 YZ 평면에 글자 quad를 만들고
			// billboard 월드행렬로 카메라를 바라보게 함
			Vertices.push_back(FFontVertex(FVector(0.0f, X0, Y1), FVector2(Glyph.U0, Glyph.V0)));
			Vertices.push_back(FFontVertex(FVector(0.0f, X1, Y1), FVector2(Glyph.U1, Glyph.V0)));
			Vertices.push_back(FFontVertex(FVector(0.0f, X1, Y0), FVector2(Glyph.U1, Glyph.V1)));
			Vertices.push_back(FFontVertex(FVector(0.0f, X0, Y0), FVector2(Glyph.U0, Glyph.V1)));

			Indices.push_back(BaseIndex + 0);
			Indices.push_back(BaseIndex + 1);
			Indices.push_back(BaseIndex + 2);
			Indices.push_back(BaseIndex + 0);
			Indices.push_back(BaseIndex + 2);
			Indices.push_back(BaseIndex + 3);
		}

		PenX += Glyph.Advance;
	}

	if (Vertices.empty() || Indices.empty())
	{
		return;
	}

	EnsureDynamicBuffers(
		static_cast<uint32>(Vertices.size()),
		static_cast<uint32>(Indices.size())
	);

	if (!DynamicVertexBuffer || !DynamicIndexBuffer)
	{
		return;
	}

	D3D11_MAPPED_SUBRESOURCE Mapped = {};

	if (SUCCEEDED(DeviceContext->Map(DynamicVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
	{
		std::memcpy(Mapped.pData, Vertices.data(), sizeof(FFontVertex) * Vertices.size());
		DeviceContext->Unmap(DynamicVertexBuffer, 0);
	}

	if (SUCCEEDED(DeviceContext->Map(DynamicIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
	{
		std::memcpy(Mapped.pData, Indices.data(), sizeof(uint32) * Indices.size());
		DeviceContext->Unmap(DynamicIndexBuffer, 0);
	}

	const FMatrix Billboard = FMatrix::MakeBillboard(WorldPosition, CameraPosition);
	const FMatrix Scale = FMatrix::MakeScale(WorldScale);
	const FMatrix World = Scale * Billboard;

	UpdateObjectCB(World);
	UpdateTextCB(Color);

	FontVS->Bind(DeviceContext);
	FontPS->Bind(DeviceContext);

	ID3D11Buffer* VSConstantBuffers[2] = { FrameConstantBuffer, ObjectConstantBuffer };
	DeviceContext->VSSetConstantBuffers(0, 2, VSConstantBuffers);

	ID3D11Buffer* PSConstantBuffers[1] = { TextConstantBuffer };
	DeviceContext->PSSetConstantBuffers(2, 1, PSConstantBuffers);

	ID3D11ShaderResourceView* SRV = Atlas.GetTextureSRV();
	ID3D11SamplerState* Sampler = Atlas.GetSamplerState();
	DeviceContext->PSSetShaderResources(0, 1, &SRV);
	DeviceContext->PSSetSamplers(0, 1, &Sampler);

	const float BlendFactor[4] = { 0, 0, 0, 0 };
	DeviceContext->OMSetBlendState(AlphaBlendState, BlendFactor, 0xffffffff);
	DeviceContext->RSSetState(NoCullRasterizerState);

	UINT Stride = sizeof(FFontVertex);
	UINT Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &DynamicVertexBuffer, &Stride, &Offset);
	DeviceContext->IASetIndexBuffer(DynamicIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	DeviceContext->DrawIndexed(static_cast<UINT>(Indices.size()), 0, 0);

	ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
	DeviceContext->PSSetShaderResources(0, 1, NullSRV);

	DeviceContext->OMSetBlendState(nullptr, BlendFactor, 0xffffffff);
	DeviceContext->RSSetState(nullptr);
}