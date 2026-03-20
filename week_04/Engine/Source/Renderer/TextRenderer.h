#pragma once

#include "CoreMinimal.h"
#include "Renderer/FontAtlas.h"
#include "Renderer/FontVertex.h"
#include <d3d11.h>
#include <memory>

class FVertexShader;
class FPixelShader;
class FShaderResource;

class ENGINE_API CTextRenderer
{
public:
	CTextRenderer() = default;
	~CTextRenderer();

	bool Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext);
	void Release();

	void Begin(const FMatrix& InView, const FMatrix& InProjection, const FVector& InCameraPosition);

	void DrawTextBillboard(
		const FString& Text,
		const FVector& WorldPosition,
		float WorldScale,
		const FVector4& Color
	);

private:
	bool CreateShaders();
	bool CreateConstantBuffers();
	bool CreateRenderStates();

	void EnsureDynamicBuffers(uint32 VertexCount, uint32 IndexCount);
	void UpdateFrameCB();
	void UpdateObjectCB(const FMatrix& WorldMatrix);
	void UpdateTextCB(const FVector4& Color);

	TArray<uint32> DecodeToCodepoints(const FString& Text) const;

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;

	std::shared_ptr<FVertexShader> FontVS;
	std::shared_ptr<FPixelShader> FontPS;

	FFontAtlas Atlas;

	ID3D11Buffer* FrameConstantBuffer = nullptr;   // b0
	ID3D11Buffer* ObjectConstantBuffer = nullptr;  // b1
	ID3D11Buffer* TextConstantBuffer = nullptr;    // b2

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