#pragma once
#include "CoreMinimal.h"
#include "Windows.h"
#include "Core/FTimer.h"
#include <functional>

class AActor;
class UScene;
class CRenderer;
class CShaderManager;
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

	void SetSelectedActor(AActor* InActor) { SelectedActor = InActor; }
	AActor* GetSelectedActor() const { return SelectedActor; }

	void OnResize(int32 Width, int32 Height);

	using FRenderCallback = std::function<void(CRenderer*)>;
	void SetPostRenderCallback(FRenderCallback InCallback) { PostRenderCallback = std::move(InCallback); }

private:
	void ProcessCameraInput(float DeltaTime);
	void Physics(float DeltaTime);
	void GameLogic(float DeltaTime);
	void Render();

private:
	CRenderer* Renderer = nullptr;
	CShaderManager* ShaderManager = nullptr;
	CInputManager* InputManager = nullptr;
	UScene* Scene = nullptr;
	AActor* SelectedActor = nullptr;

	FRenderCallback PostRenderCallback;

	FTimer Timer;
	int32 WindowWidth = 0;
	int32 WindowHeight = 0;
};
