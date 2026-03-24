#include "Renderer/AxisRenderer.h"
#include "Renderer/ShaderType.h"
#include "Core/Paths.h"
#include <cstring>

struct FCameraConstantBuffer
{
	FVector CameraPos;
	float GridSize;
	float LineThickness;
	FVector Padding;
};

CAxisRenderer::~CAxisRenderer()
{
	Release();
}

bool CAxisRenderer::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext)
{
	Release();

	Device = InDevice;
	DeviceContext = InDeviceContext;

	if (!Device || !DeviceContext)
		return false;

	if (!CreateShaders())
		return false;

	if (!CreateConstantBuffers())
		return false;

	if (!CreateRenderStates())
		return false;

	return true;
}

void CAxisRenderer::Release()
{
	AxisVS.reset();
	AxisPS.reset();

	if (FrameConstantBuffer)
	{
		FrameConstantBuffer->Release();
		FrameConstantBuffer = nullptr;
	}

	if (CameraConstantBuffer)
	{
		CameraConstantBuffer->Release();
		CameraConstantBuffer = nullptr;
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

	Device = nullptr;
	DeviceContext = nullptr;
}

bool CAxisRenderer::CreateShaders()
{
	const std::wstring ShaderDir = FPaths::ShaderDir();
	const std::wstring VSPath = ShaderDir + L"AxisVertexShader.hlsl";
	const std::wstring PSPath = ShaderDir + L"AxisPixelShader.hlsl";

	auto VSResource = FShaderResource::GetOrCompile(VSPath.c_str(), "main", "vs_5_0");
	if (!VSResource)
		return false;

	auto PSResource = FShaderResource::GetOrCompile(PSPath.c_str(), "main", "ps_5_0");
	if (!PSResource)
		return false;

	// ⚠️ Input Layout 없음 (SV_VertexID 사용)
	AxisVS = FVertexShader::Create(Device, VSResource);
	if (!AxisVS)
		return false;

	AxisPS = FPixelShader::Create(Device, PSResource);
	if (!AxisPS)
		return false;

	return true;
}

bool CAxisRenderer::CreateConstantBuffers()
{
	D3D11_BUFFER_DESC Desc = {};
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	Desc.ByteWidth = sizeof(FFrameConstantBuffer);
	if (FAILED(Device->CreateBuffer(&Desc, nullptr, &FrameConstantBuffer)))
		return false;

	Desc.ByteWidth = sizeof(FCameraConstantBuffer);
	if (FAILED(Device->CreateBuffer(&Desc, nullptr, &CameraConstantBuffer)))
		return false;

	return true;
}

bool CAxisRenderer::CreateRenderStates()
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
		return false;

	return true;
}

void CAxisRenderer::Begin(const FMatrix& InView, const FMatrix& InProjection, const FVector& InCameraPosition)
{
	ViewMatrix = InView;
	ProjectionMatrix = InProjection;
	CameraPosition = InCameraPosition;

	UpdateFrameCB();
}

void CAxisRenderer::UpdateFrameCB()
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

void CAxisRenderer::UpdateCameraCB(float GridSize, float LineThickness)
{
	FCameraConstantBuffer CBData;
	CBData.CameraPos = CameraPosition;
	CBData.GridSize = GridSize;
	CBData.LineThickness = LineThickness;
	CBData.Padding = FVector::ZeroVector;

	D3D11_MAPPED_SUBRESOURCE Mapped = {};
	if (SUCCEEDED(DeviceContext->Map(CameraConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
	{
		std::memcpy(Mapped.pData, &CBData, sizeof(CBData));
		DeviceContext->Unmap(CameraConstantBuffer, 0);
	}
}

void CAxisRenderer::Draw(float GridSize, float LineThickness)
{
	if (!Device || !DeviceContext || !AxisVS || !AxisPS)
		return;

	UpdateCameraCB(GridSize, LineThickness);

	AxisVS->Bind(DeviceContext);
	AxisPS->Bind(DeviceContext);

	// Constant Buffers
	ID3D11Buffer* VS_CB[1] = { FrameConstantBuffer };
	DeviceContext->VSSetConstantBuffers(0, 1, VS_CB);

	ID3D11Buffer* PS_CB[1] = { CameraConstantBuffer };
	DeviceContext->VSSetConstantBuffers(2, 1, PS_CB);
	DeviceContext->PSSetConstantBuffers(2, 1, PS_CB);

	// States
	const float BlendFactor[4] = { 0,0,0,0 };
	DeviceContext->OMSetBlendState(AlphaBlendState, BlendFactor, 0xffffffff);
	DeviceContext->RSSetState(NoCullRasterizerState);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ⭐ 핵심: VertexID 기반
	DeviceContext->Draw(18, 0);

	DeviceContext->OMSetBlendState(nullptr, BlendFactor, 0xffffffff);
	DeviceContext->RSSetState(nullptr);
}