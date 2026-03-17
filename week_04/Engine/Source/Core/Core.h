#pragma once
#include "CoreMinimal.h"
#include "Windows.h"
#include "Core/FTimer.h"

class UScene;
class CRenderer;
class CInputManager;

class ENGINE_API CCore
{
public:
	CCore() = default;
	~CCore();

	CCore(const CCore&) = delete;
	CCore(CCore&&) = delete;
	CCore& operator=(const CCore&) = delete;
	CCore& operator=(CCore&&) = delete;
	bool Initialize(HWND Hwnd, int32 Width, int32 Height);
	void Release();

	void Tick();
	void Tick(float DeltaTime);

	void ProcessInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);

	UScene* GetScene() const { return Scene; }
	CRenderer* GetRenderer() const { return Renderer; }
	CInputManager* GetInputManager() const { return InputManager; }

	void OnResize(int32 Width, int32 Height);

private:
	void ProcessCameraInput(float DeltaTime);
	void Physics(float DeltaTime);
	void GameLogic(float DeltaTime);
	void Render();

private:
	CRenderer* Renderer = nullptr;
	CInputManager* InputManager = nullptr;
	UScene* Scene = nullptr;

	FTimer Timer;
};
