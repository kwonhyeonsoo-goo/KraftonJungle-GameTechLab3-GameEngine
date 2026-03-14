#include "InputManager.h"
#include "Camera/Camera.h"

void CInputManager::ProcessMessage(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	switch (Msg)
	{
	case WM_RBUTTONDOWN:
		bRightMouseDown = true;
		GetCursorPos(&LastMousePos);
		SetCapture(Hwnd);
		break;

	case WM_RBUTTONUP:
		bRightMouseDown = false;
		ReleaseCapture();
		break;
	}
}

void CInputManager::Tick(float DeltaTime, CCamera* Camera)
{
	if (!Camera)
	{
		return;
	}

	// Keyboard: WASD + QE
	if (GetAsyncKeyState('W') & 0x8000) Camera->MoveForward(DeltaTime);
	if (GetAsyncKeyState('S') & 0x8000) Camera->MoveForward(-DeltaTime);
	if (GetAsyncKeyState('D') & 0x8000) Camera->MoveRight(DeltaTime);
	if (GetAsyncKeyState('A') & 0x8000) Camera->MoveRight(-DeltaTime);
	if (GetAsyncKeyState('E') & 0x8000) Camera->MoveUp(DeltaTime);
	if (GetAsyncKeyState('Q') & 0x8000) Camera->MoveUp(-DeltaTime);

	// Mouse: Right-click drag -> Camera rotation
	if (bRightMouseDown)
	{
		POINT CurrentMousePos;
		GetCursorPos(&CurrentMousePos);
		float DeltaX = static_cast<float>(CurrentMousePos.x - LastMousePos.x);
		float DeltaY = static_cast<float>(CurrentMousePos.y - LastMousePos.y);
		LastMousePos = CurrentMousePos;

		Camera->Rotate(DeltaX * 0.2f, -DeltaY * 0.2f);
	}
}
