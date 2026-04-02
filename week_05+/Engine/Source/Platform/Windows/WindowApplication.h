#pragma once
#include "CoreMinimal.h"
#include "WindowTypes.h"

class FWindow;

class ENGINE_API FWindowApplication
{
public:
	static FWindowApplication& Get();

	bool Create(HINSTANCE InInstance, const WCHAR* ClassName = L"JungleEngineWindow");
	void Destroy();

	FWindow* MakeWindow(const WCHAR* Title, int Width, int Height, int X = 100, int Y = 100);
	bool CreateMainWindow(const WCHAR* Title, int Width, int Height, int X = 100, int Y = 100);

	// Returns false when WM_QUIT received
	bool PumpMessages();

	HINSTANCE GetInstance() const { return Instance; }
	const WCHAR* GetClassName() const { return WindowClassName; }

	FWindow* GetMainWindow() const { return MainWindow; }
	HWND GetHwnd() const;
	int32 GetWindowWidth() const;
	int32 GetWindowHeight() const;

	void AddMessageFilter(FWndProcFilter Filter);
	void SetOnResizeCallback(FOnResizeCallback Callback);
	void ShowWindow();

private:
	FWindowApplication() = default;
	~FWindowApplication() = default;

	static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	HINSTANCE Instance = nullptr;
	WNDCLASSEX WindowClass = {};
	WCHAR WindowClassName[128] = {};
	bool bClassRegistered = false;
	FWindow* MainWindow = nullptr;

	// HWND -> FWindow mapping
	static TMap<HWND, FWindow*> WindowMap;

	void RegisterWindow(HWND Hwnd, FWindow* Window);
	void UnregisterWindow(HWND Hwnd);

	friend class FWindow;
};
