#include "Window.h"
#include "Debug/EngineLog.h"
#include <windows.h>

int SWindow::NextWindowId = 0;

void SWindow::Update(uint32 Msg, uint32 Wparam, uint64 Lparam)
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

	if (coord.PointX >= minX && coord.PointY <= maxX
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
	UE_LOG("New Bar Size: H%d W%d X%d Y%d", Rect.Height, Rect.Width, Rect.TopLeftX, Rect.TopLeftY);

}
