#include "EngineTest.h"
#include "Core/Core.h"
#include "Renderer/Renderer.h"
#include "UI/EditorGUI.h"

#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

static CCore* GCore = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		if (GCore && GCore->GetRenderer() && wParam != SIZE_MINIMIZED)
		{
			GCore->GetRenderer()->OnResize(LOWORD(lParam), HIWORD(lParam));
		}
		return 0;
	default:
		if (GCore)
		{
			GCore->ProcessInput(hWnd, msg, wParam, lParam);
		}
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	// Window
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"TestWindow";
	RegisterClassEx(&wc);

	HWND hwnd = CreateWindowEx(
		0, L"TestWindow", L"Renderer Test",
		WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720,
		nullptr, nullptr, hInstance, nullptr
	);
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Core
	CCore Core;
	GCore = &Core;
	if (!Core.Initialize(hwnd, 1280, 720))
		return -1;

	// ImGui

	CEditorGUI EditorGUI;
	EditorGUI.Initialize(Core.GetRenderer());



	// Timing
	LARGE_INTEGER Frequency, LastTime, CurrentTime;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&LastTime);

	// Main loop
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if (Core.GetRenderer()->IsOccluded())
				continue;

			QueryPerformanceCounter(&CurrentTime);
			float DeltaTime = static_cast<float>(CurrentTime.QuadPart - LastTime.QuadPart)
				/ static_cast<float>(Frequency.QuadPart);
			LastTime = CurrentTime;

			Core.Tick(DeltaTime);
		}
	}

	// Cleanup
	GCore = nullptr;
	Core.Release();

	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}
