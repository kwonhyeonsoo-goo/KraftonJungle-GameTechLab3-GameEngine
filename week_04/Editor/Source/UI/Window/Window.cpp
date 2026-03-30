#include "Window.h"
#include "Debug/EngineLog.h"
#include "UI/Viewport.h"
#include <windows.h>
#include <d3d11.h>

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
	if (newRect.Height <= 0) newRect.Height = 1;
	if (newRect.Width <= 0) newRect.Width = 1;

	//혹시 resize하다가 터지면 
	//ID3D11ShaderResourceView* RTV = Viewport->GetSRV();
	//RTV->Release();
	//ID3D11Texture2D* backBuffer;
	//SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);

	//// 새 RTV 생성
	//Device->CreateRenderTargetView(backBuffer, NULL, &RenderTargetView);

	//backBuffer->Release();
}
