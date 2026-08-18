#pragma once
#include "CoreMinimal.h"
#include "EngineAPI.h"
#include "d3d11.h"
#include "Math/Rect.h"

class ENGINE_API FViewport
{
public:
	FViewport(FRect InRect);
	virtual ~FViewport();

	bool GetMousePositionInViewport(int32 WindowMouseX, int32 WindowMouseY, int32& OutViewportX, int32& OutViewportY, int32& OutWidth, int32& OutHeight) const;
	bool ContainsPoint(int32 WindowMouseX, int32 WindowMouseY) const;
	void SetRect(FRect InRect);
	const FRect& GetRect() const { return ViewportRect; }
	void SetHovered(bool bInHovered) { bHovered = bInHovered; }
	void SetFocused(bool bInFocused) { bFocused = bInFocused; }
	void SetVisible(bool bInVisible) { bVisible = bInVisible; }
	bool IsHovered() const { return bHovered; }
	bool IsFocused() const { return bFocused; }
	bool IsVisible() const { return bVisible; }
	D3D11_VIEWPORT GetD3D11Viewport() const;

private:
	FRect ViewportRect;
	bool bHovered = false;
	bool bFocused = false;
	bool bVisible = false;
};
