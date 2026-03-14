#pragma once

#include "CoreMinimal.h"
#include "Renderer/RenderCommand.h"
#include <d3d11.h>
#include <vector>

struct FMeshData;

class ENGINE_API CRenderer
{
public:
	CRenderer() = default;
	~CRenderer();

	bool Initialize(HWND Hwnd, int Width, int Height);
	void BeginFrame();
	void EndFrame();
	void Release();

	// 커맨드 수집
	void AddCommand(const FRenderCommand& Command);

	// 수집된 커맨드 정렬 후 실행
	void ExecuteCommands();

	ID3D11Device* GetDevice() const { return Device; }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }

private:
	bool CreateConstantBuffer();
	void UpdateConstantBuffer(const FMatrix& WorldMatrix, const FMatrix& ViewProj);

	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;
	ID3D11Buffer* ConstantBuffer = nullptr;
	ID3D11RasterizerState* RasterizerState = nullptr;
	D3D11_VIEWPORT Viewport = {};

	std::vector<FRenderCommand> CommandList;

	// 매 프레임 외부에서 설정
public:
	FMatrix ViewProjectionMatrix;
};
