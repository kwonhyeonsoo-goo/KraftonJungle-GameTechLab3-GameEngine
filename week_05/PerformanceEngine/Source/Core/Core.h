#pragma once

#include <memory>
#include <Windows.h>

#include "Picking/PickingSystem.h"
#include "Visibility/VisibilitySystem.h"

#include "Types/PlatformTypes.h"

class FCamera;
class FD3D11RHI;
class FHudRenderer;
class FInput;
class FPickingSystem;
class FScene;
class FSceneRenderer;
class FStatsSystem;
class FVisibilitySystem;
class FWindowsWindow;

struct FCoreInitArgs
{
	FWindowsWindow* MainWindow = nullptr;
	HWND Hwnd = nullptr;
	int32 Width = 0;
	int32 Height = 0;
};

class FCore
{
public:
	FCore();
	~FCore();

	FCore(const FCore&) = delete;
	FCore(FCore&&) = delete;
	FCore& operator=(const FCore&) = delete;
	FCore& operator=(FCore&&) = delete;

	bool Initialize(const FCoreInitArgs& Args);
	void Tick();
	void Shutdown();
	bool HandleMessage(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);
	void HandleResize(int32 Width, int32 Height);

	void Release();

private:
	void BeginFrame();
	void EndFrame();
	bool LoadDefaultScene();

private:
	std::unique_ptr<FD3D11RHI> RHI;
	std::unique_ptr<FInput> Input;
	std::unique_ptr<FCamera> Camera;
	std::unique_ptr<FScene> Scene;
	std::unique_ptr<FSceneRenderer> SceneRenderer;
	std::unique_ptr<FHudRenderer> HudRenderer;
	std::unique_ptr<FVisibilitySystem> VisibilitySystem;
	std::unique_ptr<FPickingSystem> PickingSystem;
	std::unique_ptr<FStatsSystem> StatsSystem;

	FVisibilityResults VisibilityResults;
	FPickState PickState;

	bool bInitialized = false;
};
