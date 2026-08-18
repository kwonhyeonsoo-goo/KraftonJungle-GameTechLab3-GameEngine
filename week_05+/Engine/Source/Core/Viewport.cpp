#include "Viewport.h"
#include "Core/ViewportClient.h"

FViewport::FViewport(FRect InRect)
{
	SetRect(InRect);
}

FViewport::~FViewport() = default;

bool FViewport::GetMousePositionInViewport(int32 WindowMouseX, int32 WindowMouseY, int32& OutViewportX, int32& OutViewportY, int32& OutWidth, int32& OutHeight) const
{
	if (!ContainsPoint(WindowMouseX, WindowMouseY))
	{
		return false;
	}

	OutViewportX = WindowMouseX - static_cast<int32>(ViewportRect.Position.X);
	OutViewportY = WindowMouseY - static_cast<int32>(ViewportRect.Position.Y);
	OutWidth = static_cast<int32>(ViewportRect.Size.X);
	OutHeight = static_cast<int32>(ViewportRect.Size.Y);
	return true;
}

bool FViewport::ContainsPoint(int32 WindowMouseX, int32 WindowMouseY) const
{
	if (!bVisible || ViewportRect.Size.X <= 0.0f || ViewportRect.Size.Y <= 0.0f)
	{
		return false;
	}

	return ViewportRect.ContainsPoint(FPoint(static_cast<float>(WindowMouseX), static_cast<float>(WindowMouseY)));
}

void FViewport::SetRect(FRect InRect)
{
	ViewportRect = InRect;
	bVisible = (ViewportRect.Size.X > 0.0f && ViewportRect.Size.Y > 0.0f);

	if (!bVisible)
	{
		bHovered = false;
		bFocused = false;
	}
}

D3D11_VIEWPORT FViewport::GetD3D11Viewport() const
{
	D3D11_VIEWPORT Viewport = {};
	Viewport.TopLeftX = ViewportRect.Position.X;
	Viewport.TopLeftY = ViewportRect.Position.Y;
	Viewport.Width = ViewportRect.Size.X;
	Viewport.Height = ViewportRect.Size.Y;
	Viewport.MaxDepth = 1.f;
	Viewport.MinDepth = 0.f;
	return Viewport;
}
