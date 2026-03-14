#include "EngineTest.h"
#include "Core/Core.h"
#include "Camera/Camera.h"
#include "Object/Scene/Scene.h"

// ─── 입력 상태 ───
static bool bRightMouseDown = false;
static POINT LastMousePos = {};

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_RBUTTONDOWN:
		bRightMouseDown = true;
		GetCursorPos(&LastMousePos);
		SetCapture(hWnd);
		return 0;
	case WM_RBUTTONUP:
		bRightMouseDown = false;
		ReleaseCapture();
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	// ─── 윈도우 생성 ───
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
	ShowWindow(hwnd, SW_SHOW);

	// ─── Core 초기화 ───
	CCore Core;
	if (!Core.Initialize(hwnd, 1280, 720))
		return -1;

	CCamera* Camera = Core.GetScene()->GetCamera();

	// ─── 타이밍 ───
	LARGE_INTEGER Frequency, LastTime, CurrentTime;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&LastTime);

	// ─── 메인 루프 ───
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
			// 델타 타임
			QueryPerformanceCounter(&CurrentTime);
			float DeltaTime = static_cast<float>(CurrentTime.QuadPart - LastTime.QuadPart)
				/ static_cast<float>(Frequency.QuadPart);
			LastTime = CurrentTime;

			// ─── 키보드 입력 (WASD + QE) ───
			if (GetAsyncKeyState('W') & 0x8000) Camera->MoveForward(DeltaTime);
			if (GetAsyncKeyState('S') & 0x8000) Camera->MoveForward(-DeltaTime);
			if (GetAsyncKeyState('D') & 0x8000) Camera->MoveRight(DeltaTime);
			if (GetAsyncKeyState('A') & 0x8000) Camera->MoveRight(-DeltaTime);
			if (GetAsyncKeyState('E') & 0x8000) Camera->MoveUp(DeltaTime);
			if (GetAsyncKeyState('Q') & 0x8000) Camera->MoveUp(-DeltaTime);

			// ─── 마우스 우클릭 드래그 → 카메라 회전 ───
			if (bRightMouseDown)
			{
				POINT CurrentMousePos;
				GetCursorPos(&CurrentMousePos);
				float DeltaX = static_cast<float>(CurrentMousePos.x - LastMousePos.x);
				float DeltaY = static_cast<float>(CurrentMousePos.y - LastMousePos.y);
				LastMousePos = CurrentMousePos;

				Camera->Rotate(DeltaX * 0.2f, -DeltaY * 0.2f);
			}

			Core.Tick(DeltaTime);
		}
	}

	// ─── 정리 ───
	Core.Release();

	return 0;
}
