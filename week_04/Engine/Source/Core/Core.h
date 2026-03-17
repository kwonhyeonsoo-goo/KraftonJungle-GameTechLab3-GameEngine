#pragma once

#include "CoreMinimal.h"
#include "Windows.h"
#include "Core/FTimer.h"
#include "Core/ViewportClient.h"
#include "Scene/SceneContext.h"
#include "Scene/SceneTypes.h"
#include "Renderer/Renderer.h"
#include "Input/InputManager.h"
#include <memory>

class AActor;
class UScene;
class ObjectManager;

class ENGINE_API CCore
{
public:
	CCore() = default;
	~CCore();

	CCore(const CCore&) = delete;
	CCore(CCore&&) = delete;
	CCore& operator=(const CCore&) = delete;
	CCore& operator=(CCore&&) = delete;

	bool Initialize(HWND Hwnd, int32 Width, int32 Height, ESceneType StartupSceneType = ESceneType::Game);
	void Release();

	void Tick();
	void Tick(float DeltaTime);

	void ProcessInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);

	UScene* GetScene() const { return GetActiveScene(); }
	UScene* GetActiveScene() const { return ActiveSceneContext ? ActiveSceneContext->Scene : nullptr; }
	UScene* GetEditorScene() const { return EditorSceneContext.Scene; }
	UScene* GetGameScene() const { return GameSceneContext.Scene; }
	UScene* GetPreviewScene(const FString& ContextName) const;

	const FSceneContext* GetActiveSceneContext() const { return ActiveSceneContext; }
	const TArray<std::unique_ptr<FEditorSceneContext>>& GetPreviewSceneContexts() const { return PreviewSceneContexts; }
	CRenderer* GetRenderer() const { return Renderer.get(); }
	CInputManager* GetInputManager() const { return InputManager.get(); }
	IViewportClient* GetViewportClient() const { return ViewportClient; }
	const FTimer& GetTimer() const { return Timer; }

	void SetSelectedActor(AActor* InActor);
	AActor* GetSelectedActor() const;
	void SetViewportClient(IViewportClient* InViewportClient);
	FEditorSceneContext* CreatePreviewSceneContext(const FString& ContextName);
	bool DestroyPreviewSceneContext(const FString& ContextName);
	void ActivateEditorScene() { ActiveSceneContext = EditorSceneContext.Scene ? &EditorSceneContext : nullptr; }
	void ActivateGameScene() { ActiveSceneContext = GameSceneContext.Scene ? &GameSceneContext : nullptr; }
	bool ActivatePreviewScene(const FString& ContextName);

	void OnResize(int32 Width, int32 Height);

private:
	bool CreateSceneContext(FSceneContext& Context, const FString& ContextName, ESceneType SceneType, float AspectRatio, bool bInitializeDefaultScene = true);
	void DestroySceneContext(FSceneContext& Context);
	void DestroySceneContext(FEditorSceneContext& Context);
	FEditorSceneContext* GetActiveEditorSceneContext();
	const FEditorSceneContext* GetActiveEditorSceneContext() const;
	FEditorSceneContext* FindPreviewSceneContext(const FString& ContextName);
	const FEditorSceneContext* FindPreviewSceneContext(const FString& ContextName) const;
	void ProcessCameraInput(float DeltaTime);
	void Physics(float DeltaTime);
	void GameLogic(float DeltaTime);
	void Render();
	void RegisterConsoleVariables();

private:
	std::unique_ptr<CRenderer> Renderer;
	std::unique_ptr<CInputManager> InputManager;
	ObjectManager* ObjManager = nullptr;
	FSceneContext GameSceneContext;
	FEditorSceneContext EditorSceneContext;
	TArray<std::unique_ptr<FEditorSceneContext>> PreviewSceneContexts;
	FSceneContext* ActiveSceneContext = nullptr;
	IViewportClient* ViewportClient = nullptr;

	FTimer Timer;
	double LastGCTime = 0.0;
	static constexpr double GCInterval = 30.0;
	int32 WindowWidth = 0;
	int32 WindowHeight = 0;
};
