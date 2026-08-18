#include "Input.h"

#include <cstring>

namespace
{
	POINT MakeClientPoint(LPARAM InLParam)
	{
		POINT Point = {};
		Point.x = static_cast<short>(LOWORD(InLParam));
		Point.y = static_cast<short>(HIWORD(InLParam));
		return Point;
	}

	void RegisterRawMouseInput(HWND InHwnd, bool& bOutRegistered)
	{
		if (bOutRegistered || InHwnd == nullptr)
		{
			return;
		}

		RAWINPUTDEVICE RawInputDevice = {};
		RawInputDevice.usUsagePage = 0x01;
		RawInputDevice.usUsage = 0x02;
		RawInputDevice.dwFlags = 0;
		RawInputDevice.hwndTarget = InHwnd;

		bOutRegistered = RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RAWINPUTDEVICE)) == TRUE;
	}
}

void FInput::ProcessMessage(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	RegisterRawMouseInput(Hwnd, bRawMouseRegistered);

	switch (Msg)
	{
	case WM_KEYDOWN:
		if (WParam < MAX_KEYS)
			EventQueue.push_back({ EInputEventType::KeyDown, static_cast<int32>(WParam) });
		break;

	case WM_KEYUP:
		if (WParam < MAX_KEYS)
			EventQueue.push_back({ EInputEventType::KeyUp, static_cast<int32>(WParam) });
		break;

	case WM_LBUTTONDOWN:
		EventQueue.push_back({ EInputEventType::MouseButtonDown, MOUSE_LEFT });
		MousePositionClient = MakeClientPoint(LParam);
		SetCapture(Hwnd);
		break;

	case WM_LBUTTONUP:
		EventQueue.push_back({ EInputEventType::MouseButtonUp, MOUSE_LEFT });
		MousePositionClient = MakeClientPoint(LParam);
		ReleaseCapture();
		break;

	case WM_RBUTTONDOWN:
		EventQueue.push_back({ EInputEventType::MouseButtonDown, MOUSE_RIGHT });
		MousePositionClient = MakeClientPoint(LParam);
		GetCursorPos(&LastMousePos);
		bTrackingMouse = true;
		SetCapture(Hwnd);
		break;

	case WM_RBUTTONUP:
		EventQueue.push_back({ EInputEventType::MouseButtonUp, MOUSE_RIGHT });
		MousePositionClient = MakeClientPoint(LParam);
		bTrackingMouse = false;
		ReleaseCapture();
		break;

	case WM_MBUTTONDOWN:
		EventQueue.push_back({ EInputEventType::MouseButtonDown, MOUSE_MIDDLE });
		MousePositionClient = MakeClientPoint(LParam);
		break;

	case WM_MBUTTONUP:
		EventQueue.push_back({ EInputEventType::MouseButtonUp, MOUSE_MIDDLE });
		MousePositionClient = MakeClientPoint(LParam);
		break;

	case WM_MOUSEMOVE:
		MousePositionClient = MakeClientPoint(LParam);
		break;

	case WM_INPUT:
		if (bTrackingMouse)
		{
			UINT RawInputSize = 0;
			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(LParam), RID_INPUT, nullptr, &RawInputSize, sizeof(RAWINPUTHEADER)) == 0
				&& RawInputSize > 0)
			{
				std::vector<BYTE> RawInputBytes(RawInputSize);
				if (GetRawInputData(
					reinterpret_cast<HRAWINPUT>(LParam),
					RID_INPUT,
					RawInputBytes.data(),
					&RawInputSize,
					sizeof(RAWINPUTHEADER)) == RawInputSize)
				{
					const RAWINPUT* RawInput = reinterpret_cast<const RAWINPUT*>(RawInputBytes.data());
					if (RawInput->header.dwType == RIM_TYPEMOUSE)
					{
						PendingMouseDeltaX += static_cast<float>(RawInput->data.mouse.lLastX);
						PendingMouseDeltaY += static_cast<float>(RawInput->data.mouse.lLastY);
					}
				}
			}
		}
		break;
	}
}

void FInput::Tick()
{
	// Save previous frame state
	std::memcpy(PrevKeyState, KeyState, sizeof(KeyState));
	std::memcpy(PrevMouseButtonState, MouseButtonState, sizeof(MouseButtonState));

	// Flush event queue
	for (const FInputEvent& Event : EventQueue)
	{
		switch (Event.Type)
		{
		case EInputEventType::KeyDown:
			KeyState[Event.KeyOrButton] = true;
			break;
		case EInputEventType::KeyUp:
			KeyState[Event.KeyOrButton] = false;
			break;
		case EInputEventType::MouseButtonDown:
			MouseButtonState[Event.KeyOrButton] = true;
			break;
		case EInputEventType::MouseButtonUp:
			MouseButtonState[Event.KeyOrButton] = false;
			break;
		}
	}
	EventQueue.clear();

	// Mouse delta
	if (bTrackingMouse)
	{
		MouseDeltaX = PendingMouseDeltaX;
		MouseDeltaY = PendingMouseDeltaY;
		PendingMouseDeltaX = 0.0f;
		PendingMouseDeltaY = 0.0f;
	}
	else
	{
		MouseDeltaX = 0.0f;
		MouseDeltaY = 0.0f;
		PendingMouseDeltaX = 0.0f;
		PendingMouseDeltaY = 0.0f;
	}
}

bool FInput::IsKeyDown(int32 Key) const
{
	if (Key < 0 || Key >= MAX_KEYS) return false;
	return KeyState[Key];
}

bool FInput::IsKeyPressed(int32 Key) const
{
	if (Key < 0 || Key >= MAX_KEYS) return false;
	return KeyState[Key] && !PrevKeyState[Key];
}

bool FInput::IsKeyReleased(int32 Key) const
{
	if (Key < 0 || Key >= MAX_KEYS) return false;
	return !KeyState[Key] && PrevKeyState[Key];
}

bool FInput::IsMouseButtonDown(int32 Button) const
{
	if (Button < 0 || Button >= MAX_MOUSE_BUTTONS) return false;
	return MouseButtonState[Button];
}

bool FInput::IsMouseButtonPressed(int32 Button) const
{
	if (Button < 0 || Button >= MAX_MOUSE_BUTTONS) return false;
	return MouseButtonState[Button] && !PrevMouseButtonState[Button];
}

bool FInput::IsMouseButtonReleased(int32 Button) const
{
	if (Button < 0 || Button >= MAX_MOUSE_BUTTONS) return false;
	return !MouseButtonState[Button] && PrevMouseButtonState[Button];
}
