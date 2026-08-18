#pragma once

#include <memory>
#include <Windows.h>

#include "Picking/PickingSystem.h"
#include "Visibility/VisibilitySystem.h"

#include "Types/PlatformTypes.h"

class FGrid;
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
class FSceneGraph;
class FEditorUI;

class FSceneLoader;
class FGizmo;
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

	FD3D11RHI* GetRHI() const { return RHI.get(); }
	FCamera* GetCamera() const { return Camera.get(); }
	FScene* GetScene() const { return Scene.get(); }
	FPickingSystem* GetPickingSystem() const { return PickingSystem.get(); }
	FSceneGraph* GetSceneGraph() const { return SceneGraph.get(); }
	FVisibilitySystem* GetVisibilitySystem() const { return VisibilitySystem.get(); }

	FPickState& GetPickState() { return PickState; }

	const FScenePrimitiveRuntimeData* GetSelectedPrimitiveData() const { return SelectedPrimitiveData; }

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
	std::unique_ptr<FGrid> Grid;
	std::unique_ptr<FSceneGraph> SceneGraph;
	std::unique_ptr<FEditorUI> EditorUI;
	std::unique_ptr<FGizmo> Gizmo;
	FVisibilityResults VisibilityResults;
	FPickState PickState;

	const FScenePrimitiveRuntimeData* SelectedPrimitiveData = nullptr;

	std::unique_ptr<FSceneLoader> SceneLoader;
	bool bInitialized = false;
};
