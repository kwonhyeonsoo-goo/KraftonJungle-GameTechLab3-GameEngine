#pragma once

#include "CoreMinimal.h"
#include "Renderer/RenderCommand.h"
#include <d3d11.h>
#include <vector>
#include <functional>
#include "ShaderManager.h"
#include "PrimitiveVertex.h"
class FPixelShader;
struct FMeshData;

using FGUICallback = std::function<void()>;

class ENGINE_API CRenderer
{
public:
	CRenderer() = default;
	~CRenderer();

	bool Initialize(HWND Hwnd, int32 Width, int32 Height);
	void BeginFrame();
	void EndFrame();
	void Release();
	bool IsOccluded();
	void OnResize(int32 NewWidth, int32 NewHeight);
	bool bSwapChainOccluded = false;

	// GUI callbacks (ImGui 등 외부 GUI 시스템 연동)
	void SetGUICallbacks(
		FGUICallback InInit,
		FGUICallback InShutdown,
		FGUICallback InNewFrame,
		FGUICallback InRender,
		FGUICallback InPostPresent = nullptr
	);
	void SetGUIUpdateCallback(FGUICallback InUpdate);

	// 커맨드 수집
	void AddCommand(const FRenderCommand& Command);
	// 수집된 커맨드 정렬 후 실행
	void ExecuteCommands();
	void SetViewProjection(const FMatrix& VP) { ViewProjectionMatrix = VP; }

	// 라인 렌더링
	void DrawLine(const FVector& Start, const FVector& End, const FVector4& Color);
	void ExecuteLineCommands();

	// 아웃라인 렌더링 (Stencil 기반)
	bool InitOutlineResources();
	void RenderOutline(FMeshData* Mesh, const FMatrix& WorldMatrix, float OutlineScale = 1.05f);


	FMatrix GetViewProjectionMatrix() { return ViewProjectionMatrix; }
	ID3D11Device* GetDevice() const { return Device; }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }
	ID3D11RenderTargetView* GetRenderTargetView() const { return RenderTargetView; }
	IDXGISwapChain* GetSwapChain() const { return SwapChain; };
	HWND GetHwnd() const { return Hwnd; }
private:
	bool CreateConstantBuffer();
	void UpdateConstantBuffer(const FMatrix& WorldMatrix, const FMatrix& ViewProj);

	HWND Hwnd = nullptr;
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;
	ID3D11Buffer* ConstantBuffer = nullptr;
	ID3D11RasterizerState* RasterizerState = nullptr;
	D3D11_VIEWPORT Viewport = {};

	TArray<FRenderCommand> CommandList;
	TArray<FPrimitiveVertex> LineVertices;
	ID3D11Buffer* LineVertexBuffer = nullptr;
	ID3D11DepthStencilState* LineDepthState = nullptr;

	// 아웃라인 리소스
	ID3D11DepthStencilState* StencilWriteState = nullptr;
	ID3D11DepthStencilState* StencilTestState = nullptr;
	std::shared_ptr<FPixelShader> OutlinePS;

	// GUI callbacks
	FGUICallback GUIInit;
	FGUICallback GUIShutdown;
	FGUICallback GUINewFrame;
	FGUICallback GUIUpdate;
	FGUICallback GUIRender;
	FGUICallback GUIPostPresent;

	// 매 프레임 외부에서 설정
public:
	FMatrix ViewProjectionMatrix;
	CShaderManager ShaderManager;
};
