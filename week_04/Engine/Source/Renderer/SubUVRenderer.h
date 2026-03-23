#pragma once

#include "CoreMinimal.h"
#include "Renderer/TextureVertex.h"
#include <d3d11.h>
#include <memory>

class FVertexShader;
class FPixelShader;
class FShaderResource;

class ENGINE_API CSubUVRenderer
{
public:
	CSubUVRenderer() = default;
	~CSubUVRenderer();

	bool Initialize(
		ID3D11Device* InDevice,
		ID3D11DeviceContext* InDeviceContext,
		const std::wstring& TexturePath
	);

	void Release();

	void Begin(
		const FMatrix& InView,
		const FMatrix& InProjection,
		const FVector& InCameraPosition
	);

	void DrawSubUV(
		const FMatrix& WorldMatrix,
		const FVector2& Size,
		int32 Columns,
		int32 Rows,
		int32 TotalFrames,
		int32 FirstFrame,
		int32 LastFrame,
		float FPS,
		float ElapsedTime,
		bool bLoop,
		bool bBillboard
	);

private:
	bool CreateShaders();
	bool CreateConstantBuffers();
	bool CreateRenderStates();
	bool CreateTextureAndSampler(const std::wstring& TexturePath);

	void EnsureDynamicBuffers(uint32 VertexCount, uint32 IndexCount);

	void UpdateFrameCB();
	void UpdateObjectCB(const FMatrix& WorldMatrix);
	void UpdateSubUVCB(
		const FVector2& CellSize,
		const FVector2& Offset
	);

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;

	std::shared_ptr<FVertexShader> SubUVVS;
	std::shared_ptr<FPixelShader> SubUVPS;

	ID3D11ShaderResourceView* TextureSRV = nullptr;
	ID3D11SamplerState* SamplerState = nullptr;

	ID3D11Buffer* FrameConstantBuffer = nullptr;   // b0
	ID3D11Buffer* ObjectConstantBuffer = nullptr;  // b1
	ID3D11Buffer* SubUVConstantBuffer = nullptr;   // b2

	ID3D11Buffer* DynamicVertexBuffer = nullptr;
	ID3D11Buffer* DynamicIndexBuffer = nullptr;

	uint32 DynamicVertexCapacity = 0;
	uint32 DynamicIndexCapacity = 0;

	ID3D11BlendState* AlphaBlendState = nullptr;
	ID3D11RasterizerState* NoCullRasterizerState = nullptr;

	FMatrix ViewMatrix = FMatrix::Identity;
	FMatrix ProjectionMatrix = FMatrix::Identity;
	FVector CameraPosition = FVector::ZeroVector;
};