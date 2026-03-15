#pragma once
#include "CoreMinimal.h"
#include "Windows.h"
#include <vector>

enum class EInputEventType : unsigned char
{
	KeyDown,
	KeyUp,
	MouseButtonDown,
	MouseButtonUp,
};

struct FInputEvent
{
	EInputEventType Type;
	int KeyOrButton;
};

class ENGINE_API CInputManager
{
public:
	CInputManager() = default;
	~CInputManager() = default;

	CInputManager(const CInputManager&) = delete;
	CInputManager(CInputManager&&) = delete;
	CInputManager& operator=(const CInputManager&) = delete;
	CInputManager& operator=(CInputManager&&) = delete;

	void ProcessMessage(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);
	void Tick();

	bool IsKeyDown(int Key) const;
	bool IsKeyPressed(int Key) const;
	bool IsKeyReleased(int Key) const;

	bool IsMouseButtonDown(int Button) const;
	bool IsMouseButtonPressed(int Button) const;
	bool IsMouseButtonReleased(int Button) const;

	float GetMouseDeltaX() const { return MouseDeltaX; }
	float GetMouseDeltaY() const { return MouseDeltaY; }

	static constexpr int MOUSE_LEFT = 0;
	static constexpr int MOUSE_RIGHT = 1;
	static constexpr int MOUSE_MIDDLE = 2;

private:
	static constexpr int MAX_KEYS = 256;
	static constexpr int MAX_MOUSE_BUTTONS = 3;

	// Event queue (filled by WndProc, flushed in Tick)
	std::vector<FInputEvent> EventQueue;

	bool KeyState[MAX_KEYS] = {};
	bool PrevKeyState[MAX_KEYS] = {};

	bool MouseButtonState[MAX_MOUSE_BUTTONS] = {};
	bool PrevMouseButtonState[MAX_MOUSE_BUTTONS] = {};

	float MouseDeltaX = 0.0f;
	float MouseDeltaY = 0.0f;
	POINT LastMousePos = {};
	bool bTrackingMouse = false;
};
