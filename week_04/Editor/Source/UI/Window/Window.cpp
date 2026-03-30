#include "Window.h"
#include "Debug/EngineLog.h"
#include <windows.h>

int SWindow::NextWindowId = 0;

void SWindow::UpdateInput(uint32 Msg, uint32 Wparam, uint64 Lparam)
{
	if (Msg == WM_LBUTTONDOWN) {
		POINTS mousePosition = MAKEPOINTS(Lparam);
		FPoint point = { mousePosition.x, mousePosition.y };
		IsHover(point);
	}
}

SWindow::SWindow()
{
	WindowId = SWindow::NextWindowId++;

}

SWindow::~SWindow()
{
}

void SWindow::Initialize(FRect rect)
{
	Rect = rect;
}

bool SWindow::IsHover(FPoint coord) const
{
	float minX = Rect.TopLeftX;
	float maxX = Rect.TopLeftX + Rect.Width;
	float minY = Rect.TopLeftY;
	float maxY = Rect.TopLeftY + Rect.Height;

	if (coord.PointX >= minX && coord.PointX <= maxX
		&& coord.PointY >= minY && coord.PointY <= maxY)
	{
		UE_LOG("Current Window ID : %d", WindowId);
		return true;
	}

	return false;
}

void SWindow::UpdateNewSize(FRect newRect)
{
	Rect = newRect;
	//UE_LOG("SWindow_New Bar Size: H%f W%f X%f Y%f", Rect.Height, Rect.Width, Rect.TopLeftX, Rect.TopLeftY);

}
