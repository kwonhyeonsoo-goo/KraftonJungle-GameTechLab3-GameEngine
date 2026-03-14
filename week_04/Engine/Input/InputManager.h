#pragma once
#include "CoreMinimal.h"
#include "Windows.h"

class CCamera;

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

	void Tick(float DeltaTime, CCamera* Camera);

private:
	bool bRightMouseDown = false;
	POINT LastMousePos = {};
};
