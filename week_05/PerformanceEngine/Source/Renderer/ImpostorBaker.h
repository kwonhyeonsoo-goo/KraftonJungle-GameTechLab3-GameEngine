#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <string>

#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Types/String.h"

class FStaticMesh;

// 3D 메쉬를 여러 각도에서 촬영하여 2D 텍스처 아틀라스로 구워내는 "오프라인 사진관" 클래스
class FImpostorBaker
{
public:
	FImpostorBaker();
	~FImpostorBaker();

	// 베이커 초기화 (예: 아틀라스 해상도 2048, 16x16 그리드 = 256방향)
	// 그러면 각 사진(타일)의 크기는 2048 / 16 = 128x128 픽셀이 됩니다.
	bool Initialize(ID3D11Device* InDevice, int32 InAtlasResolution = 2048, int32 InGridSize = 16);
	void Release();

	// 타겟 메쉬를 촬영석 중앙에 앉히고 256방향에서 찰칵! 찍은 뒤 파일로 저장합니다.
	bool Bake(ID3D11DeviceContext* InContext, FStaticMesh* InMesh, const std::wstring& InOutputFilePath);

	// 접근자
	int32 GetAtlasResolution() const { return AtlasResolution; }
	int32 GetGridSize() const { return GridSize; }
	int32 GetTileSize() const { return TileSize; }

private:
	// 카메라 방향을 계산하기 위해 아까 만든 OctDecode 수학을 사용할 헬퍼 함수
	FVector GetCameraDirection(int32 GridX, int32 GridY) const;

	// 메쉬의 크기에 맞게 카메라의 거리와 직교 투영(Orthographic) 크기를 잡아주는 함수
	FMatrix CalculateCameraTransform(const FVector& BoundsCenter, float BoundsRadius, const FVector& CameraDir) const;

private:
	int32 AtlasResolution = 2048;
	int32 GridSize = 16;
	int32 TileSize = 128; // AtlasResolution / GridSize

	// Albedo (알베도 색상) + Alpha (투명도)
	Microsoft::WRL::ComPtr<ID3D11Texture2D> AlbedoAtlasTex;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> AlbedoRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AlbedoSRV;

	// Normal (빛 반사를 위한 월드 노말) + Depth (입체감을 위한 깊이 정보 압축)
	Microsoft::WRL::ComPtr<ID3D11Texture2D> NormalDepthAtlasTex;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> NormalDepthRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> NormalDepthSRV;

	// 베이킹 전용 깊이 버퍼 (촬영할 때 앞뒤 판별용)
	Microsoft::WRL::ComPtr<ID3D11Texture2D> DepthStencilTex;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DepthDSV;

	Microsoft::WRL::ComPtr<ID3D11Buffer> BakeConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> BakeVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> BakePS;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> BakeInputLayout;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> BakeSampler;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> BakeRasterizerState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> BakeDepthStencilState;
	// 타일 1개를 찍을 때마다 이동시킬 카메라 렌즈(뷰포트)
	D3D11_VIEWPORT TileViewport;
};