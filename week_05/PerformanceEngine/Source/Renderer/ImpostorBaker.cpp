#include "ImpostorBaker.h"
#include "Math/MathUtility.h"
#include "Math/Vector2.h"
#include "StaticMesh/StaticMesh.h"
#include <cassert>
#include <wincodec.h>
#include "Graphics/D3D11/D3D11Utils.h"
#include <ScreenGrab.h>
#include <cmath>
namespace
{
	FVector OctDecode(const FVector2& InUV)
	{
		FVector Res(InUV.X, InUV.Y, 1.0f - std::abs(InUV.X) - std::abs(InUV.Y));
		if (Res.Z < 0.0f)
		{
			float OldX = Res.X;
			Res.X = (1.0f - std::abs(Res.Y)) * (OldX >= 0.0f ? 1.0f : -1.0f);
			Res.Y = (1.0f - std::abs(OldX)) * (Res.Y >= 0.0f ? 1.0f : -1.0f);
		}
		Res.Normalize();
		return Res;
	}
}

FImpostorBaker::FImpostorBaker() = default;

FImpostorBaker::~FImpostorBaker()
{
	Release();
}

void FImpostorBaker::Release()
{
	AlbedoRTV.Reset();
	AlbedoSRV.Reset();
	AlbedoAtlasTex.Reset();

	NormalDepthRTV.Reset();
	NormalDepthSRV.Reset();
	NormalDepthAtlasTex.Reset();

	DepthDSV.Reset();
	DepthStencilTex.Reset();

	BakeSampler.Reset();
	BakeRasterizerState.Reset();
	BakeDepthStencilState.Reset();
}

bool FImpostorBaker::Initialize(ID3D11Device* InDevice, int32 InAtlasResolution, int32 InGridSize)
{
	Release();

	AtlasResolution = InAtlasResolution;
	GridSize = InGridSize;
	TileSize = AtlasResolution / GridSize;

	// 1. Albedo (색상 + 투명도) 텍스처 및 RTV 생성
	D3D11_TEXTURE2D_DESC TexDesc = {};
	TexDesc.Width = AtlasResolution;
	TexDesc.Height = AtlasResolution;
	TexDesc.MipLevels = 1;
	TexDesc.ArraySize = 1;
	TexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	TexDesc.SampleDesc.Count = 1;
	TexDesc.Usage = D3D11_USAGE_DEFAULT;
	TexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	if (FAILED(InDevice->CreateTexture2D(&TexDesc, nullptr, AlbedoAtlasTex.GetAddressOf()))) return false;
	if (FAILED(InDevice->CreateRenderTargetView(AlbedoAtlasTex.Get(), nullptr, AlbedoRTV.GetAddressOf()))) return false;
	if (FAILED(InDevice->CreateShaderResourceView(AlbedoAtlasTex.Get(), nullptr, AlbedoSRV.GetAddressOf()))) return false;

	// 2. Normal + Depth 텍스처 및 RTV 생성 (정밀도를 위해 FLOAT16 사용 권장)
	TexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	if (FAILED(InDevice->CreateTexture2D(&TexDesc, nullptr, NormalDepthAtlasTex.GetAddressOf()))) return false;
	if (FAILED(InDevice->CreateRenderTargetView(NormalDepthAtlasTex.Get(), nullptr, NormalDepthRTV.GetAddressOf()))) return false;
	if (FAILED(InDevice->CreateShaderResourceView(NormalDepthAtlasTex.Get(), nullptr, NormalDepthSRV.GetAddressOf()))) return false;

	// 3. 깊이(DepthStencil) 버퍼 생성 (2048x2048 통째로 씁니다)
	D3D11_TEXTURE2D_DESC DepthDesc = {};
	DepthDesc.Width = AtlasResolution;
	DepthDesc.Height = AtlasResolution;
	DepthDesc.MipLevels = 1;
	DepthDesc.ArraySize = 1;
	DepthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthDesc.SampleDesc.Count = 1;
	DepthDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	if (FAILED(InDevice->CreateTexture2D(&DepthDesc, nullptr, DepthStencilTex.GetAddressOf()))) return false;
	if (FAILED(InDevice->CreateDepthStencilView(DepthStencilTex.Get(), nullptr, DepthDSV.GetAddressOf()))) return false;

	// 타일 촬영용 뷰포트 기본 세팅
	TileViewport.Width = static_cast<float>(TileSize);
	TileViewport.Height = static_cast<float>(TileSize);
	TileViewport.MinDepth = 0.0f;
	TileViewport.MaxDepth = 1.0f;
	D3D11_BUFFER_DESC cbDesc = {};
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.ByteWidth = sizeof(FMatrix) * 2;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(InDevice->CreateBuffer(&cbDesc, nullptr, BakeConstantBuffer.GetAddressOf()))) return false;
	static constexpr char BakeShaderSource[] = R"(
	cbuffer CB_BakeMatrix : register(b0)
	{
		row_major float4x4 WorldViewProj;
		row_major float4x4 World;
	};

	Texture2D DiffuseMap : register(t0);
	SamplerState Sampler : register(s0);

	struct VS_INPUT
	{
		float3 Pos : POSITION;
		float2 UV : TEXCOORD0;
	};

	struct PS_INPUT
	{
		float4 Pos : SV_POSITION;
		float2 UV : TEXCOORD0;
		float3 WorldPos : TEXCOORD1;
	};

	struct PS_OUTPUT
	{
		float4 Albedo : SV_Target0;
		float4 NormalDepth : SV_Target1;
	};

	PS_INPUT VSMain(VS_INPUT input)
	{
		PS_INPUT output;
		float4 worldPos = mul(float4(input.Pos, 1.0f), World);
		output.WorldPos = worldPos.xyz;
		output.Pos = mul(float4(input.Pos, 1.0f), WorldViewProj);
		output.UV = input.UV;
		return output;
	}

	PS_OUTPUT PSMain(PS_INPUT input)
	{
		PS_OUTPUT output;
		float4 baseColor = DiffuseMap.Sample(Sampler, input.UV);
		clip(baseColor.a - 0.33f); // 투명한 배경은 버림
		
		output.Albedo = baseColor;
		
		// 픽셀의 공간 미분값을 사용해 삼각형의 표면 노말(Face Normal)을 역산합니다.
		float3 dx = ddx(input.WorldPos);
		float3 dy = ddy(input.WorldPos);
		float3 normal = normalize(cross(dy, dx)); 
		
		// 노말 [-1, 1] 범위를 색상용 [0, 1] 범위로 압축
		float3 encodedNormal = normal * 0.5f + 0.5f;
		
		// Depth값
		float depth = input.Pos.z;
		
		output.NormalDepth = float4(encodedNormal, depth);
		return output;
	}
	)";

	TComPtr<ID3DBlob> VSBlob;
	TComPtr<ID3DBlob> PSBlob;

	if (!D3D11Utils::CompileShaderFromSource(BakeShaderSource, "VSMain", "vs_5_0", VSBlob, "Impostor VS") ||
		!D3D11Utils::CompileShaderFromSource(BakeShaderSource, "PSMain", "ps_5_0", PSBlob, "Impostor PS"))
	{
		OutputDebugStringA("Impostor Shader Compile Failed!\n");
		return false;
	}

	if (FAILED(InDevice->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), nullptr, BakeVS.GetAddressOf())) ||
		FAILED(InDevice->CreatePixelShader(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), nullptr, BakePS.GetAddressOf())))
	{
		return false;
	}

	// 입력 레이아웃 (유저님의 FStaticMeshVertex 구조에 맞춤)
	const D3D11_INPUT_ELEMENT_DESC InputElements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, static_cast<UINT>(offsetof(FStaticMeshVertex, Position)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, static_cast<UINT>(offsetof(FStaticMeshVertex, TexCoord)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	if (FAILED(InDevice->CreateInputLayout(InputElements, _countof(InputElements), VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), BakeInputLayout.GetAddressOf())))
	{
		return false;
	}

	// 베이킹 전용 샘플러 (텍스처 샘플링에 필수)
	D3D11_SAMPLER_DESC SampDesc = {};
	SampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	SampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	SampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	SampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	SampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	if (FAILED(InDevice->CreateSamplerState(&SampDesc, BakeSampler.GetAddressOf())))
	{
		return false;
	}

	// 베이킹 전용 래스터라이저 (양면 렌더링)
	D3D11_RASTERIZER_DESC RastDesc = {};
	RastDesc.FillMode = D3D11_FILL_SOLID;
	RastDesc.CullMode = D3D11_CULL_NONE;
	RastDesc.DepthClipEnable = TRUE;
	if (FAILED(InDevice->CreateRasterizerState(&RastDesc, BakeRasterizerState.GetAddressOf())))
	{
		return false;
	}

	// 베이킹 전용 뎁스 스텐실 (뎁스 테스트 ON, 쓰기 ON)
	D3D11_DEPTH_STENCIL_DESC DSDesc = {};
	DSDesc.DepthEnable = TRUE;
	DSDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DSDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	if (FAILED(InDevice->CreateDepthStencilState(&DSDesc, BakeDepthStencilState.GetAddressOf())))
	{
		return false;
	}

	return true;
}

FVector FImpostorBaker::GetCameraDirection(int32 GridX, int32 GridY) const
{
	// 1. 그리드 좌표를 [0, 1] 범위의 UV로 변환 (각 타일의 중앙을 바라보도록 +0.5f)
	float U = (static_cast<float>(GridX) + 0.5f) / static_cast<float>(GridSize);
	float V = (static_cast<float>(GridY) + 0.5f) / static_cast<float>(GridSize);

	// 2. UV를 [-1, 1] 범위의 NDC(정규화 좌표)로 변환
	// (DirectX는 V(Y축)가 아래로 갈수록 커지므로 뒤집어 줍니다)
	FVector2 NDC(U * 2.0f - 1.0f, 1.0f - V * 2.0f);

	// 3. OctDecode 수학을 이용해 3D 방향 벡터로 디코딩
	return OctDecode(NDC);
}

FMatrix FImpostorBaker::CalculateCameraTransform(const FVector& BoundsCenter, float BoundsRadius, const FVector& CameraDir) const
{

	float Radius = BoundsRadius;
	if (Radius <= 0.01f)
	{
		Radius = 1.0f;
	}

	float CameraDistance = Radius * 2.0f + 10.0f;
	FVector EyePosition = BoundsCenter + (CameraDir * CameraDistance);

	FVector UpVector = FVector::UpVector;
	if (std::abs(FVector::DotProduct(CameraDir, FVector::UpVector)) > 0.999f)
	{
		UpVector = FVector::RightVector;
	}

	FMatrix ViewMatrix = FMatrix::MakeLookAt(EyePosition, BoundsCenter, UpVector);


	float OrthoSize = Radius * 4.0f;
	FMatrix ProjMatrix = FMatrix::MakeOrthographicLH(OrthoSize, OrthoSize, 0.1f, CameraDistance + Radius * 2.0f);

	return ViewMatrix * ProjMatrix;
}

bool FImpostorBaker::Bake(ID3D11DeviceContext* InContext, FStaticMesh* InMesh, const std::wstring& InOutputFilePath)
{
	if (!InMesh || !AlbedoRTV) return false;

	// 도화지(Render Target)와 깊이 버퍼 지우기
	// 배경은 투명(Alpha=0)으로 지워야 나중에 임포스터 판때기가 네모낳게 안 보입니다.
	const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	ID3D11RenderTargetView* RTVs[2] = { AlbedoRTV.Get(), NormalDepthRTV.Get() };

	InContext->ClearRenderTargetView(AlbedoRTV.Get(), ClearColor);
	InContext->ClearRenderTargetView(NormalDepthRTV.Get(), ClearColor);
	InContext->ClearDepthStencilView(DepthDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// 렌더 타겟 2개(색상, 노말/깊이)를 동시에 바인딩 (MRT: Multiple Render Targets)
	InContext->OMSetRenderTargets(2, RTVs, DepthDSV.Get());

	// 베이킹 전용 렌더 스테이트 설정
	InContext->RSSetState(BakeRasterizerState.Get());
	InContext->OMSetDepthStencilState(BakeDepthStencilState.Get(), 0);
	ID3D11SamplerState* Samplers[] = { BakeSampler.Get() };
	InContext->PSSetSamplers(0, 1, Samplers);
	InContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 셰이더 바인딩 (루프 밖에서 한 번만)
	InContext->VSSetShader(BakeVS.Get(), nullptr, 0);
	InContext->PSSetShader(BakePS.Get(), nullptr, 0);
	InContext->IASetInputLayout(BakeInputLayout.Get());

	// 메쉬 버퍼 바인딩 (루프 밖에서 한 번만)
	UINT Stride = sizeof(FStaticMeshVertex);
	UINT Offset = 0;
	ID3D11Buffer* VertexBuffer = InMesh->GetVertexBuffer();
	InContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
	ID3D11Buffer* IndexBuffer = InMesh->GetIndexBuffer();
	InContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	FVector BoundsCenter = InMesh->GetBoundsMin() + (InMesh->GetBoundsMax() - InMesh->GetBoundsMin()) * 0.5f;
	float BoundsRadius = FVector::Dist(InMesh->GetBoundsMax(), BoundsCenter);

	// 256방향 촬영 루프
	for (int32 y = 0; y < GridSize; ++y)
	{
		for (int32 x = 0; x < GridSize; ++x)
		{
			// 타일별로 뎁스 클리어 (이전 타일 뎁스가 영향 안 주게)
			D3D11_RECT ScissorRect;
			ScissorRect.left = x * TileSize;
			ScissorRect.top = y * TileSize;
			ScissorRect.right = ScissorRect.left + TileSize;
			ScissorRect.bottom = ScissorRect.top + TileSize;

			TileViewport.TopLeftX = static_cast<float>(x * TileSize);
			TileViewport.TopLeftY = static_cast<float>(y * TileSize);
			InContext->RSSetViewports(1, &TileViewport);

			FVector CamDir = GetCameraDirection(x, y);
			FMatrix ViewProj = CalculateCameraTransform(BoundsCenter, BoundsRadius, CamDir);
			struct FBakeCB { FMatrix WVP; FMatrix World; } CBData;

			CBData.WVP = ViewProj.GetTransposed();
			CBData.World = FMatrix::Identity.GetTransposed();

			D3D11_MAPPED_SUBRESOURCE MappedResource = {};
			if (SUCCEEDED(InContext->Map(BakeConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
			{
				memcpy(MappedResource.pData, &CBData, sizeof(FBakeCB));
				InContext->Unmap(BakeConstantBuffer.Get(), 0);
			}

			ID3D11Buffer* CBs[] = { BakeConstantBuffer.Get() };
			InContext->VSSetConstantBuffers(0, 1, CBs);

			// 섹션(머티리얼)별로 순회하며 그리기
			for (const FStaticMesh::FSection& Section : InMesh->GetSections())
			{
				ID3D11ShaderResourceView* TextureView = InMesh->GetMaterialTexture(Section.MaterialIndex);
				if (TextureView)
				{
					InContext->PSSetShaderResources(0, 1, &TextureView);
				}
				InContext->DrawIndexed(Section.IndexCount, Section.IndexStart, 0);
			}
		}
	}
	InContext->OMSetRenderTargets(0, nullptr, nullptr);
	std::wstring AlbedoPath = InOutputFilePath + L"_Albedo.png";
	std::wstring NormalPath = InOutputFilePath + L"_NormalDepth.png";

	// Albedo (색상) 텍스처 저장
	HRESULT hr1 = DirectX::SaveWICTextureToFile(
		InContext,
		AlbedoAtlasTex.Get(),
		GUID_ContainerFormatPng,
		AlbedoPath.c_str()
	);

	// Normal + Depth 텍스처 저장
	HRESULT hr2 = DirectX::SaveWICTextureToFile(
		InContext,
		NormalDepthAtlasTex.Get(),
		GUID_ContainerFormatPng,
		NormalPath.c_str()
	);

	char DebugMsg[512];
	if (FAILED(hr1))
	{
		sprintf_s(DebugMsg, "[ImpostorBaker] Albedo 저장 실패! HRESULT: 0x%08X (경로 문제 또는 COM 초기화 누락)\n", hr1);
		OutputDebugStringA(DebugMsg);
	}
	if (FAILED(hr2))
	{
		sprintf_s(DebugMsg, "[ImpostorBaker] NormalDepth 저장 실패! HRESULT: 0x%08X\n", hr2);
		OutputDebugStringA(DebugMsg);
	}

	if (SUCCEEDED(hr1) && SUCCEEDED(hr2))
	{
		OutputDebugStringA("[ImpostorBaker] Impostor Atlas 성공적으로 구워졌습니다!\n");
	}

	return (SUCCEEDED(hr1) && SUCCEEDED(hr2));
}