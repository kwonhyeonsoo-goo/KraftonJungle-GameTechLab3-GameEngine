#pragma once
#include <d3d11.h>
#include <CoreMinimal.h>
#include "Core/ShowFlags.h"

class FRenderer;
class IViewportClient;



class FViewport
{
public:
	~FViewport();

	// RTV 준비 + ViewportInfo 전달
	// EditorUI가 4분할 영역 계산 후 호출
	void PrepareAndUpdate(FRenderer* Renderer, HWND Hwnd,
		float RegionX, float RegionY,
		float RegionW, float RegionH);

	// ImGui::Image용 SRV
	ID3D11ShaderResourceView* GetSRV() const { return ShaderResourceView; }

	void ReleaseLevelView();
	bool GetMousePositionInViewport(int32 WindowMouseX, int32 WindowMouseY,
		int32& OutViewportX, int32& OutViewportY,
		int32& OutWidth, int32& OutHeight) const;

	bool IsHovered()  const { return bHovered; }
	bool IsFocused()  const { return bFocused; }
	bool IsVisible()  const { return bVisible; }

	void SetHovered(bool b) { bHovered = b; }
	void SetFocused(bool b) { bFocused = b; }

	void ReadyLevelView(ID3D11Device* Device, ID3D11DeviceContext* Context, uint32 Width, uint32 Height);

	void SetLinkedViewportClient(IViewportClient* InClient) { LinkedViewportClient = InClient; }

	void SetCameraViewMode(CameraViewMode mode);
	CameraViewMode GetCameraViewMode() { return CameraMode; }

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

	IViewportClient* LinkedViewportClient = nullptr;
	FShowFlags ShowFlags;

	CameraViewMode CameraMode = CameraViewMode::Perspective;
};