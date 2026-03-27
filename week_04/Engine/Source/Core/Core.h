#pragma once

#include "CoreMinimal.h"
#include "Windows.h"
#include "Core/FTimer.h"
#include "World/LevelTypes.h"
#include "Renderer/Renderer.h"
#include "Physics/PhysicsManager.h"
#include "World/LevelManager.h"
#include "World/WorldContext.h"
#include <memory>
#include "Debug/DebugDrawManager.h"
class FEnhancedInputManager;
class FInputManager;

class AActor;
class ULevel;
class ObjectManager;
class IViewportClient;

class ENGINE_API FCore
{
public:
	FCore() = default;
	~FCore();

	FCore(const FCore&) = delete;
	FCore(FCore&&) = delete;
	FCore& operator=(const FCore&) = delete;
	FCore& operator=(FCore&&) = delete;

	bool Initialize(HWND Hwnd, int32 Width, int32 Height, ELevelType StartupLevelType = ELevelType::Game);
	void Release();

	void Tick();
	void Tick(float DeltaTime);

	void ProcessInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);
	FRenderer* GetRenderer() const { return Renderer.get(); }

	IViewportClient* GetViewportClient() const { return ViewportClient; }
	FInputManager* GetInputManager() const { return InputManager; }
	const FTimer& GetTimer() const { return Timer; }

	void SetViewportClient(IViewportClient* InViewportClient);

	void OnResize(int32 Width, int32 Height);
	FEnhancedInputManager* GetEnhancedInputManager() const { return EnhancedInput; }
	float GetDeltaTime() const { return Timer.GetDeltaTime(); }

	FLevelManager* GetLevelManager() const { return LevelManager.get(); }

	// Getter
	ULevel* GetLevel() const { return LevelManager->GetActiveLevel(); }
	ULevel* GetActiveLevel() const { return LevelManager->GetActiveLevel(); }
	ULevel* GetEditorLevel() const { return LevelManager->GetEditorLevel(); }
	ULevel* GetGameLevel() const { return LevelManager->GetGameLevel(); }

	void SetSelectedActor(AActor* A) { LevelManager->SetSelectedActor(A); }
	AActor* GetSelectedActor() const { return LevelManager->GetSelectedActor(); }
	void ActivateEditorLevel() { LevelManager->ActivateEditorLevel(); }
	void ActivateGameLevel() { LevelManager->ActivateGameLevel(); }
	bool ActivatePreviewLevel(const FString& ContextName) { return LevelManager->ActivatePreviewLevel(ContextName); }

	// ===== World 접근자 =====
	UWorld* GetActiveWorld() const { return LevelManager->GetActiveWorld(); }
	UWorld* GetEditorWorld() const { return LevelManager->GetEditorWorld(); }
	UWorld* GetGameWorld() const { return LevelManager->GetGameWorld(); }
	const FWorldContext* GetActiveWorldContext() const { return LevelManager->GetActiveWorldContext(); }

private:
	void Input(float DeltaTime);
	void Physics(float DeltaTime);
	void GameLogic(float DeltaTime);
	void Render();
	void LateUpdate(float DeltaTime);
	void RegisterConsoleVariables();
	FDebugDrawManager& GetDebugDrawManager() { return DebugDrawManager; }

private:
	FDebugDrawManager DebugDrawManager;
	std::unique_ptr<FRenderer> Renderer;
	FInputManager* InputManager = nullptr;
	FEnhancedInputManager* EnhancedInput = nullptr;

	ObjectManager* ObjManager = nullptr;
	IViewportClient* ViewportClient = nullptr;
	std::unique_ptr<FLevelManager> LevelManager;

	std::unique_ptr<FPhysicsManager> PhysicsManager;

	FTimer Timer;
	double LastGCTime = 0.0;
	double GCInterval = 30.0;
	int32 WindowWidth = 0;
	int32 WindowHeight = 0;

	FRenderCommandQueue CommandQueue;
};
