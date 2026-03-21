#pragma once

#include "CoreMinimal.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/TextRenderer.h"
#include "Renderer/SubUVRenderer.h"
#include <d3d11.h>
#include <functional>
#include <memory>
#include "ShaderManager.h"
#include "PrimitiveVertex.h"
class FPixelShader;
class FMaterial;
struct FMeshData;
class UScene;

using FGUICallback = std::function<void()>;
class CRenderer;
using FPostRenderCallback = std::function<void(CRenderer*)>;

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
	void SetSceneRenderTarget(ID3D11RenderTargetView* InRenderTargetView, ID3D11DepthStencilView* InDepthStencilView, const D3D11_VIEWPORT& InViewport);
	void ClearSceneRenderTarget();

	void SetVSync(bool bEnable) { bVSyncEnabled = bEnable; }
	bool IsVSyncEnabled() const { return bVSyncEnabled; }

	bool bSwapChainOccluded = false;

	// GUI callbacks (ImGui 등 외부 GUI 시스템 연동)
	void SetGUICallbacks(
		FGUICallback InInit,
		FGUICallback InShutdown,
		FGUICallback InNewFrame,
		FGUICallback InRender,
		FGUICallback InPostPresent = nullptr
	);
	void ClearViewportCallbacks();
	void SetGUIUpdateCallback(FGUICallback InUpdate);
	void SetPostRenderCallback(FPostRenderCallback InCallback) { PostRenderCallback = std::move(InCallback); }

	// 커맨드 큐 제출 — 큐에서 GPU 버퍼 보장 후 내부 CommandList로 이전
	void SubmitCommands(const FRenderCommandQueue& Queue);
	// 수집된 커맨드 정렬 후 실행
	void ExecuteCommands();

	// 라인 렌더링
	void DrawLine(const FVector& Start, const FVector& End, const FVector4& Color);

	// 큐브 렌더링
	void DrawCube(const FVector& Center, const FVector& BoxExtent, const FVector4& Color);
	
	void ExecuteLineCommands();

	// 아웃라인 렌더링 (Stencil 기반)
	bool InitOutlineResources();
	void RenderOutline(FMeshData* Mesh, const FMatrix& WorldMatrix, float OutlineScale = 1.05f);

	FMaterial* GetDefaultMaterial() const { return DefaultMaterial.get(); }

	size_t GetPrevCommandCount() const { return PrevCommandCount; }

	ID3D11Device* GetDevice() const { return Device; }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }
	ID3D11RenderTargetView* GetRenderTargetView() const { return RenderTargetView; }
	IDXGISwapChain* GetSwapChain() const { return SwapChain; };
	HWND GetHwnd() const { return Hwnd; }

private:
	void AddCommand(const FRenderCommand& Command);
	bool CreateDeviceAndSwapChain(HWND InHwnd, int32 Width, int32 Height);
	bool CreateRenderTargetAndDepthStencil(int32 Width, int32 Height);
	bool CreateConstantBuffers();
	void UpdateFrameConstantBuffer();
	void UpdateObjectConstantBuffer(const FMatrix& WorldMatrix);

	HWND Hwnd = nullptr;
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;
	ID3D11Buffer* FrameConstantBuffer = nullptr;
	ID3D11Buffer* ObjectConstantBuffer = nullptr;
	FMatrix ViewMatrix;
	FMatrix ProjectionMatrix;
	ID3D11RasterizerState* RasterizerState = nullptr;
	ID3D11RasterizerState* NoCullRasterizerState = nullptr;
	D3D11_VIEWPORT Viewport = {};
	ID3D11RenderTargetView* SceneRenderTargetView = nullptr;
	ID3D11DepthStencilView* SceneDepthStencilView = nullptr;
	D3D11_VIEWPORT SceneViewport = {};
	bool bUseSceneRenderTargetOverride = false;
	bool bVSyncEnabled = false;

	TArray<FRenderCommand> CommandList;
	TArray<FTextRenderCommand> TextCommandList;
	TArray<FSubUVRenderCommand> SubUVCommandList;

	size_t PrevCommandCount = 0;
	TArray<FPrimitiveVertex> LineVertices;
	ID3D11Buffer* LineVertexBuffer = nullptr;
	UINT LineVertexBufferSize = 0;
	ID3D11DepthStencilState* LineDepthState = nullptr;
	ID3D11DepthStencilState* OverlayDepthState = nullptr;

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
	FPostRenderCallback PostRenderCallback;

	// 기본 Material (셰이더 미지정 시 사용)
	std::shared_ptr<FMaterial> DefaultMaterial;

	// 텍스처 렌더링 테스트용 (임시)
	CTextRenderer TextRenderer;
	bool bEnableTextRenderTest = false;

	// SubUV 렌더링
	CSubUVRenderer SubUVRenderer;

	// 매 프레임 외부에서 설정
public:
	CShaderManager ShaderManager;
};
