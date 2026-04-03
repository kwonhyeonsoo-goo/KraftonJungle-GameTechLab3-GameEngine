#include "Grid.h"

#include <array>
#include <filesystem>

#include "Camera/Camera.h"
#include "Graphics/D3D11/D3D11Utils.h"
#include "Graphics/D3D11/D3D11RHI.h"
#include "Math/Matrix.h"
#include "Math/Vector4.h"
#include "Types/PlatformTypes.h"

namespace
{
	constexpr float GridHalfExtent = 5000.0f;

	struct FGridVertex
	{
		float X;
		float Y;
		float Z;
	};

	constexpr std::array<FGridVertex, 4> GridVertices =
	{{
		{ -GridHalfExtent, -GridHalfExtent, 0.0f },
		{ -GridHalfExtent,  GridHalfExtent, 0.0f },
		{  GridHalfExtent, -GridHalfExtent, 0.0f },
		{  GridHalfExtent,  GridHalfExtent, 0.0f },
	}};

	constexpr std::array<FGridVertex, 2> ZAxisVertices =
	{{
		{ 0.0f, 0.0f, -GridHalfExtent },
		{ 0.0f, 0.0f,  GridHalfExtent },
	}};

	constexpr std::array<uint16_t, 6> GridIndices =
	{{
		0, 1, 2,
		2, 1, 3
	}};

	struct alignas(16) FGridConstants
	{
		FMatrix World = FMatrix::Identity;
		FMatrix View = FMatrix::Identity;
		FMatrix Projection = FMatrix::Identity;

		FVector4 CameraPosition = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
		FVector4 MinorLineColor = FVector4(0.35f, 0.35f, 0.35f, 0.35f);
		FVector4 MajorLineColor = FVector4(0.55f, 0.55f, 0.55f, 0.55f);
		FVector4 XAxisColor = FVector4(0.85f, 0.20f, 0.20f, 0.95f);
		FVector4 YAxisColor = FVector4(0.20f, 0.60f, 0.20f, 0.95f);

		float MinorCellSize = 1.0f;
		float MajorCellSize = 10.0f;
		float FadeDistance = 100.f;
		float Padding0 = 0.0f;
	};

	struct alignas(16) FAxisConstants
	{
		FMatrix ViewProjection = FMatrix::Identity;
		FVector4 AxisColor = FVector4(0.30f, 0.54f, 0.98f, 1.0f);
	};

	std::filesystem::path SearchForRelativePathFrom(const std::filesystem::path& InStartDirectory, const std::filesystem::path& InRelativePath)
	{
		std::filesystem::path Cursor = InStartDirectory;
		while (!Cursor.empty())
		{
			const std::filesystem::path Candidate = Cursor / InRelativePath;
			if (std::filesystem::exists(Candidate))
			{
				return std::filesystem::absolute(Candidate);
			}

			if (!Cursor.has_parent_path())
			{
				break;
			}

			const std::filesystem::path Parent = Cursor.parent_path();
			if (Parent == Cursor)
			{
				break;
			}

			Cursor = Parent;
		}

		return {};
	}

	std::filesystem::path FindGridShaderPath(const wchar_t* InFileName)
	{
		const std::array<std::filesystem::path, 2> RelativeCandidates =
		{
			std::filesystem::path(L"PerformanceEngine/Shader/Grid") / InFileName,
			std::filesystem::path(L"Shader/Grid") / InFileName,
		};

		for (const std::filesystem::path& RelativeCandidate : RelativeCandidates)
		{
			const std::filesystem::path CurrentCandidate = SearchForRelativePathFrom(std::filesystem::current_path(), RelativeCandidate);
			if (!CurrentCandidate.empty())
			{
				return CurrentCandidate;
			}
		}

		std::array<wchar_t, MAX_PATH> ModulePath = {};
		const DWORD CharacterCount = GetModuleFileNameW(nullptr, ModulePath.data(), static_cast<DWORD>(ModulePath.size()));
		if (CharacterCount == 0)
		{
			return {};
		}

		const std::filesystem::path ModuleDirectory = std::filesystem::path(ModulePath.data()).parent_path();
		for (const std::filesystem::path& RelativeCandidate : RelativeCandidates)
		{
			const std::filesystem::path CurrentCandidate = SearchForRelativePathFrom(ModuleDirectory, RelativeCandidate);
			if (!CurrentCandidate.empty())
			{
				return CurrentCandidate;
			}
		}

		return {};
	}
}

struct FGrid::FResources
{
	TComPtr<ID3D11VertexShader> VertexShader;
	TComPtr<ID3D11PixelShader> PixelShader;
	TComPtr<ID3D11VertexShader> AxisVertexShader;
	TComPtr<ID3D11PixelShader> AxisPixelShader;
	TComPtr<ID3D11InputLayout> InputLayout;
	TComPtr<ID3D11Buffer> VertexBuffer;
	TComPtr<ID3D11Buffer> IndexBuffer;
	TComPtr<ID3D11Buffer> ConstantBuffer;
	TComPtr<ID3D11Buffer> AxisVertexBuffer;
	TComPtr<ID3D11Buffer> AxisConstantBuffer;
	TComPtr<ID3D11RasterizerState> RasterizerState;
	TComPtr<ID3D11DepthStencilState> DepthStencilState;
	TComPtr<ID3D11BlendState> BlendState;
	UINT IndexCount = 0;
	UINT AxisVertexCount = 0;
};

FGrid::FGrid() = default;
FGrid::~FGrid()
{
	Release();
}

bool FGrid::Initialize(const FD3D11RHI& InRHI)
{
	Release();

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

	if (!D3D11Utils::CreateImmutableBuffer(
			Device,
			static_cast<UINT>(sizeof(FGridVertex) * GridVertices.size()),
			D3D11_BIND_VERTEX_BUFFER,
			GridVertices.data(),
			Resources->VertexBuffer)
		|| !D3D11Utils::CreateImmutableBuffer(
			Device,
			static_cast<UINT>(sizeof(uint16_t) * GridIndices.size()),
			D3D11_BIND_INDEX_BUFFER,
			GridIndices.data(),
			Resources->IndexBuffer)
		|| !D3D11Utils::CreateDynamicConstantBuffer(Device, sizeof(FGridConstants), Resources->ConstantBuffer)
		|| !D3D11Utils::CreateImmutableBuffer(
			Device,
			static_cast<UINT>(sizeof(FGridVertex) * ZAxisVertices.size()),
			D3D11_BIND_VERTEX_BUFFER,
			ZAxisVertices.data(),
			Resources->AxisVertexBuffer)
		|| !D3D11Utils::CreateDynamicConstantBuffer(Device, sizeof(FAxisConstants), Resources->AxisConstantBuffer))
	{
		Resources.reset();
		return false;
	}

	const std::filesystem::path VertexShaderPath = FindGridShaderPath(L"GridVertexShader.hlsl");
	const std::filesystem::path PixelShaderPath = FindGridShaderPath(L"GridPixelShader.hlsl");
	if (VertexShaderPath.empty() || PixelShaderPath.empty())
	{
		OutputDebugStringA("[Grid] Failed to locate grid shader files.\n");
		Resources.reset();
		return false;
	}

	TComPtr<ID3DBlob> VertexShaderBlob;
	TComPtr<ID3DBlob> PixelShaderBlob;
	TComPtr<ID3DBlob> AxisVertexShaderBlob;
	TComPtr<ID3DBlob> AxisPixelShaderBlob;
	if (!D3D11Utils::CompileShaderFromFile(VertexShaderPath, "VSMain", "vs_5_0", VertexShaderBlob, "Grid vertex shader")
		|| !D3D11Utils::CompileShaderFromFile(PixelShaderPath, "PSMain", "ps_5_0", PixelShaderBlob, "Grid pixel shader"))
	{
		Resources.reset();
		return false;
	}

	static constexpr char AxisVertexShaderSource[] = R"(
cbuffer AxisConstants : register(b0)
{
    row_major float4x4 ViewProjection;
    float4 AxisColor;
};

struct VSInput
{
    float3 Position : POSITION;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
};

VSOutput VSMain(VSInput Input)
{
    VSOutput Output;
    Output.Position = mul(float4(Input.Position, 1.0f), ViewProjection);
    Output.Color = AxisColor;
    return Output;
}
)";

	static constexpr char AxisPixelShaderSource[] = R"(
struct PSInput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR0;
};

float4 PSMain(PSInput Input) : SV_TARGET
{
    return Input.Color;
}
)";

	if (!D3D11Utils::CompileShaderFromSource(AxisVertexShaderSource, "VSMain", "vs_5_0", AxisVertexShaderBlob, "Grid axis vertex shader")
		|| !D3D11Utils::CompileShaderFromSource(AxisPixelShaderSource, "PSMain", "ps_5_0", AxisPixelShaderBlob, "Grid axis pixel shader"))
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
			Resources->PixelShader.GetAddressOf()))
		|| FAILED(Device->CreateVertexShader(
			AxisVertexShaderBlob->GetBufferPointer(),
			AxisVertexShaderBlob->GetBufferSize(),
			nullptr,
			Resources->AxisVertexShader.GetAddressOf()))
		|| FAILED(Device->CreatePixelShader(
			AxisPixelShaderBlob->GetBufferPointer(),
			AxisPixelShaderBlob->GetBufferSize(),
			nullptr,
			Resources->AxisPixelShader.GetAddressOf())))
	{
		Resources.reset();
		return false;
	}

	const D3D11_INPUT_ELEMENT_DESC InputElements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	DepthStencilDesc.StencilEnable = FALSE;

	if (FAILED(Device->CreateDepthStencilState(&DepthStencilDesc, Resources->DepthStencilState.GetAddressOf())))
	{
		Resources.reset();
		return false;
	}

	D3D11_BLEND_DESC BlendDesc = {};
	BlendDesc.RenderTarget[0].BlendEnable = TRUE;
	BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	if (FAILED(Device->CreateBlendState(&BlendDesc, Resources->BlendState.GetAddressOf())))
	{
		Resources.reset();
		return false;
	}

	Resources->IndexCount = static_cast<UINT>(GridIndices.size());
	Resources->AxisVertexCount = static_cast<UINT>(ZAxisVertices.size());
	return true;
}

void FGrid::Release()
{
	Resources.reset();
}

void FGrid::Render(const FD3D11RHI& InRHI, const FCamera& InCamera)
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

	const UINT VertexStride = sizeof(FGridVertex);
	const UINT VertexOffset = 0;
	ID3D11Buffer* VertexBuffers[] = { Resources->VertexBuffer.Get() };
	ID3D11Buffer* ConstantBuffers[] = { Resources->ConstantBuffer.Get() };
	ID3D11Buffer* AxisVertexBuffers[] = { Resources->AxisVertexBuffer.Get() };
	ID3D11Buffer* AxisConstantBuffers[] = { Resources->AxisConstantBuffer.Get() };

	const FVector CameraLocation = InCamera.GetLocation();
	FGridConstants GridConstants = {};
	GridConstants.World = FMatrix::MakeTranslation(FVector(CameraLocation.X, CameraLocation.Y, 0.0f));
	GridConstants.View = InCamera.GetViewMatrix();
	GridConstants.Projection = InCamera.GetProjectionMatrix();
	GridConstants.CameraPosition = FVector4(CameraLocation.X, CameraLocation.Y, CameraLocation.Z, 1.0f);

	if (!D3D11Utils::UpdateDynamicBuffer(DeviceContext, Resources->ConstantBuffer.Get(), GridConstants))
	{
		return;
	}

	ID3D11RenderTargetView* RenderTargets[] = { InRHI.GetBackBufferRTV() };
	const D3D11_VIEWPORT Viewport = InRHI.GetViewport();
	DeviceContext->OMSetRenderTargets(1, RenderTargets, InRHI.GetDepthStencilView());
	DeviceContext->RSSetViewports(1, &Viewport);
	DeviceContext->IASetInputLayout(Resources->InputLayout.Get());
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DeviceContext->IASetVertexBuffers(0, 1, VertexBuffers, &VertexStride, &VertexOffset);
	DeviceContext->IASetIndexBuffer(Resources->IndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	DeviceContext->VSSetShader(Resources->VertexShader.Get(), nullptr, 0);
	DeviceContext->PSSetShader(Resources->PixelShader.Get(), nullptr, 0);
	DeviceContext->VSSetConstantBuffers(0, 1, ConstantBuffers);
	DeviceContext->PSSetConstantBuffers(0, 1, ConstantBuffers);
	DeviceContext->RSSetState(Resources->RasterizerState.Get());
	DeviceContext->OMSetDepthStencilState(Resources->DepthStencilState.Get(), 0);
	DeviceContext->OMSetBlendState(Resources->BlendState.Get(), nullptr, 0xffffffffu);
	DeviceContext->DrawIndexed(Resources->IndexCount, 0, 0);

	FAxisConstants AxisConstants = {};
	AxisConstants.ViewProjection = InCamera.GetViewMatrix() * InCamera.GetProjectionMatrix();
	if (D3D11Utils::UpdateDynamicBuffer(DeviceContext, Resources->AxisConstantBuffer.Get(), AxisConstants))
	{
		DeviceContext->IASetInputLayout(Resources->InputLayout.Get());
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		DeviceContext->IASetVertexBuffers(0, 1, AxisVertexBuffers, &VertexStride, &VertexOffset);
		DeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
		DeviceContext->VSSetShader(Resources->AxisVertexShader.Get(), nullptr, 0);
		DeviceContext->PSSetShader(Resources->AxisPixelShader.Get(), nullptr, 0);
		DeviceContext->VSSetConstantBuffers(0, 1, AxisConstantBuffers);
		DeviceContext->PSSetConstantBuffers(0, 0, nullptr);
		DeviceContext->RSSetState(Resources->RasterizerState.Get());
		DeviceContext->OMSetDepthStencilState(Resources->DepthStencilState.Get(), 0);
		DeviceContext->OMSetBlendState(Resources->BlendState.Get(), nullptr, 0xffffffffu);
		DeviceContext->Draw(Resources->AxisVertexCount, 0);
	}
}
