#include "WindowApplication.h"
#include "PlatformGlobals.h"
#include "Window.h"


TMap<HWND, FWindow*> FWindowApplication::WindowMap;

FWindowApplication& FWindowApplication::Get()
{
	static FWindowApplication Instance;
	return Instance;
}

bool FWindowApplication::Create(HINSTANCE InInstance, const WCHAR* ClassName)
{
	Instance = InInstance;
	GhInstance = InInstance;

	wcscpy_s(WindowClassName, ClassName);

	WindowClass = {};
	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.lpfnWndProc = StaticWndProc;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = WindowClassName;

	if (!RegisterClassEx(&WindowClass))
	{
		return false;
	}

	bClassRegistered = true;
	return true;
}

void FWindowApplication::Destroy()
{
	if (MainWindow)
	{
		delete MainWindow;
		MainWindow = nullptr;
	}

	if (bClassRegistered)
	{
		::UnregisterClassW(WindowClassName, Instance);
		bClassRegistered = false;
	}
}

FWindow* FWindowApplication::MakeWindow(const WCHAR* Title, int Width, int Height, int X, int Y)
{
	FWindow* Window = new FWindow();
	if (!Window->Create(Instance, WindowClassName, Title, Width, Height, X, Y))
	{
		delete Window;
		return nullptr;
	}
	return Window;
}

bool FWindowApplication::CreateMainWindow(const WCHAR* Title, int Width, int Height, int X, int Y)
{
	MainWindow = MakeWindow(Title, Width, Height, X, Y);
	return MainWindow != nullptr;
}

bool FWindowApplication::PumpMessages()
{
	MSG Msg = {};
	while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (Msg.message == WM_QUIT)
		{
			return false;
		}
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return true;
}

LRESULT CALLBACK FWindowApplication::StaticWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	auto It = WindowMap.find(hWnd);
	if (It != WindowMap.end())
	{
		return It->second->HandleMessage(Msg, wParam, lParam);
	}
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

void FWindowApplication::RegisterWindow(HWND Hwnd, FWindow* Window)
{
	WindowMap[Hwnd] = Window;
}

void FWindowApplication::UnregisterWindow(HWND Hwnd)
{
	WindowMap.erase(Hwnd);
}

HWND FWindowApplication::GetHwnd() const
{
	return MainWindow ? MainWindow->GetHwnd() : nullptr;
}

int32 FWindowApplication::GetWindowWidth() const
{
	return MainWindow ? MainWindow->GetWidth() : 0;
}

int32 FWindowApplication::GetWindowHeight() const
{
	return MainWindow ? MainWindow->GetHeight() : 0;
}

void FWindowApplication::AddMessageFilter(FWndProcFilter Filter)
{
	if (MainWindow)
	{
		MainWindow->AddMessageFilter(std::move(Filter));
	}
}

void FWindowApplication::SetOnResizeCallback(FOnResizeCallback Callback)
{
	if (MainWindow)
	{
		MainWindow->SetOnResizeCallback(std::move(Callback));
	}
}

void FWindowApplication::ShowWindow()
{
	if (MainWindow)
	{
		MainWindow->Show();
	}
}
