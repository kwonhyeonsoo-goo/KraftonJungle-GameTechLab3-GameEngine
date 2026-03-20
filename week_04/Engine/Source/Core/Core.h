#pragma once

#include "CoreMinimal.h"
#include "Windows.h"
#include "Core/FTimer.h"
#include "Core/ViewportClient.h"
#include "Scene/SceneContext.h"
#include "Scene/SceneTypes.h"
#include "Renderer/Renderer.h"
#include "Input/InputManager.h"
#include "Physics/PhysicsManager.h"
#include <memory>
class CEnhancedInputManager;

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
	CRenderer* GetRenderer() const { return Renderer.get(); }

	IViewportClient* GetViewportClient() const { return ViewportClient; }
	CInputManager* GetInputManager() const { return InputManager; }
	const FTimer& GetTimer() const { return Timer; }

	void SetViewportClient(IViewportClient* InViewportClient);

	void OnResize(int32 Width, int32 Height);
	CEnhancedInputManager* GetEnhancedInputManager() const { return EnhancedInput; }
	float GetDeltaTime() const { return Timer.GetDeltaTime(); }

	FSceneManager* GetSceneManager() const { return SceneManager.get(); }

	// ���� ȣȯ ����
	UScene* GetScene() const { return SceneManager->GetActiveScene(); }
	UScene* GetActiveScene() const { return SceneManager->GetActiveScene(); }
	UScene* GetEditorScene() const { return SceneManager->GetEditorScene(); }
	UScene* GetGameScene() const { return SceneManager->GetGameScene(); }
	void SetSelectedActor(AActor* A) { SceneManager->SetSelectedActor(A); }
	AActor* GetSelectedActor() const { return SceneManager->GetSelectedActor(); }
	void ActivateEditorScene() { SceneManager->ActivateEditorScene(); }
	void ActivateGameScene() { SceneManager->ActivateGameScene(); }

	//rapper
	FEditorSceneContext* CreatePreviewSceneContext(const FString& ContextName)	{return SceneManager->CreatePreviewSceneContext(ContextName, WindowWidth, WindowHeight);}
	bool DestroyPreviewSceneContext(const FString& ContextName)	{return SceneManager->DestroyPreviewScene(ContextName);}
	bool ActivatePreviewScene(const FString& ContextName){return SceneManager->ActivatePreviewScene(ContextName);}
	const FSceneContext* GetActiveSceneContext() const{	return SceneManager->GetActiveSceneContext();}
	const TArray<std::unique_ptr<FEditorSceneContext>>& GetPreviewSceneContexts() const{return SceneManager->GetPreviewSceneContexts();}

private:
	void Input(float DeltaTime);
	void Physics(float DeltaTime);
	void GameLogic(float DeltaTime);
	void Render();
	void LateUpdate(float DeltaTime);
	void RegisterConsoleVariables();

private:
	std::unique_ptr<CRenderer> Renderer;
	CInputManager* InputManager = nullptr;
	CEnhancedInputManager* EnhancedInput = nullptr;

	ObjectManager* ObjManager = nullptr;
	IViewportClient* ViewportClient = nullptr;
	std::unique_ptr<FSceneManager> SceneManager;

	std::unique_ptr<CPhysicsManager> PhysicsManager;

	FTimer Timer;
	double LastGCTime = 0.0;
	double GCInterval = 30.0;
	int32 WindowWidth = 0;
	int32 WindowHeight = 0;
};
