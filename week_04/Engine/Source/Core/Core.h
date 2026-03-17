#pragma once
#include "CoreMinimal.h"
#include "Windows.h"
#include "Core/FTimer.h"
#include "Object/Scene/Scene.h"
#include "Renderer/Renderer.h"
#include "Input/InputManager.h"
#include <memory>
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

	UScene* GetScene() const { return Scene.get(); }
	CRenderer* GetRenderer() const { return Renderer.get(); }
	CInputManager* GetInputManager() const { return InputManager.get(); }
	const FTimer& GetTimer() const { return Timer; }

	void OnResize(int32 Width, int32 Height);

private:
	void ProcessCameraInput(float DeltaTime);
	void Physics(float DeltaTime);
	void GameLogic(float DeltaTime);
	void Render();
	void RegisterConsoleVariables();

private:
	std::unique_ptr<CRenderer> Renderer;
	std::unique_ptr<CInputManager> InputManager;
	std::unique_ptr<UScene> Scene;

	FTimer Timer;
};
