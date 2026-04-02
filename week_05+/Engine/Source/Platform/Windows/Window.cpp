#include "Window.h"
#include "WindowApplication.h"

FWindow::~FWindow()
{
	Destroy();
}

bool FWindow::Create(HINSTANCE Instance, const WCHAR* ClassName,
	const WCHAR* Title, int InWidth, int InHeight, int InX, int InY)
{
	Width = InWidth;
	Height = InHeight;
	PosX = InX;
	PosY = InY;

	// AdjustWindowRect to ensure client area matches requested size
	RECT rc = { 0, 0, Width, Height };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	int WindowWidth = rc.right - rc.left;
	int WindowHeight = rc.bottom - rc.top;

	Hwnd = CreateWindowEx(
		0, ClassName, Title,
		WS_OVERLAPPEDWINDOW,
		PosX, PosY, WindowWidth, WindowHeight,
		nullptr, nullptr, Instance, nullptr
	);

	if (!Hwnd)
	{
		return false;
	}

	FWindowApplication::Get().RegisterWindow(Hwnd, this);
	return true;
}

void FWindow::Destroy()
{
	if (Hwnd)
	{
		FWindowApplication::Get().UnregisterWindow(Hwnd);
		::DestroyWindow(Hwnd);
		Hwnd = nullptr;
	}
}

void FWindow::Show()
{
	if (Hwnd)
	{
		::ShowWindow(Hwnd, SW_SHOWDEFAULT);
		::UpdateWindow(Hwnd);
	}
}

void FWindow::Hide()
{
	if (Hwnd)
	{
		::ShowWindow(Hwnd, SW_HIDE);
	}
}

void FWindow::AddMessageFilter(FWndProcFilter Filter)
{
	MessageFilters.push_back(Filter);
}

void FWindow::SetOnResizeCallback(FOnResizeCallback Callback)
{
	OnResizeCallback = Callback;
}

LRESULT FWindow::HandleMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// Run message filters (ImGui, picking, input, etc.)
	for (auto& Filter : MessageFilters)
	{
		if (Filter(Hwnd, Msg, wParam, lParam))
		{
			return TRUE;
		}
	}

	switch (Msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			Width = LOWORD(lParam);
			Height = HIWORD(lParam);
			if (OnResizeCallback)
			{
				OnResizeCallback(Width, Height);
			}
		}
		return 0;
	}

	return DefWindowProc(Hwnd, Msg, wParam, lParam);
}
