#pragma once
#include <Windows.h>

#include "Types/Array.h"
#include "Types/PlatformTypes.h"

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
	int32 KeyOrButton;
};

class FInput
{
public:
	FInput() = default;
	~FInput() = default;

	FInput(const FInput&) = delete;
	FInput(FInput&&) = delete;
	FInput& operator=(const FInput&) = delete;
	FInput& operator=(FInput&&) = delete;

	void ProcessMessage(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);
	void Tick();

	bool IsKeyDown(int32 Key) const;
	bool IsKeyPressed(int32 Key) const;
	bool IsKeyReleased(int32 Key) const;

	bool IsMouseButtonDown(int32 Button) const;
	bool IsMouseButtonPressed(int32 Button) const;
	bool IsMouseButtonReleased(int32 Button) const;

	float GetMouseDeltaX() const { return MouseDeltaX; }
	float GetMouseDeltaY() const { return MouseDeltaY; }
	POINT GetMousePositionClient() const { return MousePositionClient; }
	int32 GetMouseX() const { return MousePositionClient.x; }
	int32 GetMouseY() const { return MousePositionClient.y; }

	static constexpr int32 MOUSE_LEFT = 0;
	static constexpr int32 MOUSE_RIGHT = 1;
	static constexpr int32 MOUSE_MIDDLE = 2;

private:
	static constexpr int32 MAX_KEYS = 256;
	static constexpr int32 MAX_MOUSE_BUTTONS = 3;

	// Event queue (filled by WndProc, flushed in Tick)
	TArray<FInputEvent> EventQueue;

	bool KeyState[MAX_KEYS] = {};
	bool PrevKeyState[MAX_KEYS] = {};

	bool MouseButtonState[MAX_MOUSE_BUTTONS] = {};
	bool PrevMouseButtonState[MAX_MOUSE_BUTTONS] = {};

	float MouseDeltaX = 0.0f;
	float MouseDeltaY = 0.0f;
	float PendingMouseDeltaX = 0.0f;
	float PendingMouseDeltaY = 0.0f;
	POINT MousePositionClient = {};
	POINT LastMousePos = {};
	bool bTrackingMouse = false;
	bool bRawMouseRegistered = false;
};
