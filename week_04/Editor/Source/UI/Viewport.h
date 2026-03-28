#pragma once
#include <d3d11.h>
#include <CoreMinimal.h>

class FCore;
class FRenderer;
class IViewportClient;

class FViewport
{
public:
	~FViewport();

	void Render(FCore* Core, FRenderer* Renderer, HWND Hwnd);
	void ReleaseLevelView();
	bool GetMousePositionInViewport(int32 WindowMouseX, int32 WindowMouseY,
		int32& OutViewportX, int32& OutViewportY,
		int32& OutWidth, int32& OutHeight) const;

	bool IsHovered()  const { return bHovered; }
	bool IsFocused()  const { return bFocused; }
	bool IsVisible()  const { return bVisible; }

	void ReadyLevelView(ID3D11Device* Device, uint32 Width, uint32 Height);

	// 이 FViewport가 정보를 밀어넣을 ViewportClient
	// 소유권 없음 — 수명은 FEditorEngine이 보장
	void SetLinkedViewportClient(IViewportClient* InClient) { LinkedViewportClient = InClient; }
	IViewportClient* GetLinkedViewportClient() const { return LinkedViewportClient; }

private:
	ID3D11Texture2D* RenderTargetTexture = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;
	ID3D11ShaderResourceView* ShaderResourceView = nullptr;
	ID3D11Texture2D* DepthStencilTexture = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;

	uint32 OffscreenWidth = 0;
	uint32 OffscreenHeight = 0;
	int32  ClientPosX = 0;
	int32  ClientPosY = 0;
	bool   bHovered = false;
	bool   bFocused = false;
	bool   bVisible = false;

	// 연결된 ViewportClient — nullptr이면 렌더 정보 전달 생략
	IViewportClient* LinkedViewportClient = nullptr;
};