#pragma once

#include "CoreMinimal.h"
#include "Renderer/Shader.h"
#include "Renderer/ShaderResource.h"
#include <d3d11.h>
#include <memory>

class FVertexShader;
class FPixelShader;

class ENGINE_API CAxisRenderer
{
public:
	CAxisRenderer() = default;
	~CAxisRenderer();

	bool Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext);
	void Release();

	void Begin(const FMatrix& InView, const FMatrix& InProjection, const FVector& InCameraPosition);

	void Draw(float GridSize, float LineThickness);

private:
	bool CreateShaders();
	bool CreateConstantBuffers();
	bool CreateRenderStates();

	void UpdateFrameCB();
	void UpdateCameraCB(float GridSize, float LineThickness);

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;

	std::shared_ptr<FVertexShader> AxisVS;
	std::shared_ptr<FPixelShader> AxisPS;

	ID3D11Buffer* FrameConstantBuffer = nullptr;   // b0
	ID3D11Buffer* CameraConstantBuffer = nullptr;  // b2

	ID3D11BlendState* AlphaBlendState = nullptr;
	ID3D11RasterizerState* NoCullRasterizerState = nullptr;

	FMatrix ViewMatrix = FMatrix::Identity;
	FMatrix ProjectionMatrix = FMatrix::Identity;
	FVector CameraPosition = FVector::ZeroVector;
};