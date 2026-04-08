#pragma once

#include "PIEState.h"
#include "Core/FEngine.h"
#include "Core/GameViewportClient.h"
#include "UI/EditorViewportClient.h"
#include "UI/EditorUI.h"
#include "UI/WindowManager.h"

class USceneComponent;

class FEditorEngine : public FEngine
{
public:
	FEditorEngine() = default;
	~FEditorEngine() override;

	bool Initialize(HINSTANCE hInstance);
	void OpenNewObj();
	void Shutdown() override;
	FWindowManager& GetWindowManager() { return WindowManager; }
	FViewportContext* CreateEditorViewportContext(const FRect& InRect, EEditorViewportType InViewportType);
	FEditorViewportClient* CreateEditorViewportClient(EEditorViewportType InViewportType, EWorldType InWorldType = EWorldType::Editor);
	void SaveEditorSettings();
	void SetViewportLayoutBounds(FRect InRect);

	EPIEState GetPIEState() const { return PIEState; }
	void SetPIEState(EPIEState NewState)
	{
		if (PIEState == NewState)
		{
			return;
		}
		if (PIEState == EPIEState::Paused)
		{
			Timer->Resume();
		}

		PIEState = NewState;
		if (PIEState == EPIEState::Paused)
		{
			Timer->Pause();
		}
	}

	FWorldContext GetEditorWorldContext() const;
	void RemoveEditorWorldContext(EWorldType WorldType);

	FEditorUI& GetEditorUI() { return EditorUI; }

	AActor* GetSelectedActor() const { return SelectedActor; }
	void SetSelectedActor(AActor* InActor) { SelectedActor = InActor; SelectedComponent = nullptr; EditorUI.SyncSelectedActorProperty(); }

	USceneComponent* GetSelectedComponent() const { return SelectedComponent; }
	void SetSelectedComponent(USceneComponent* InComponent) { SelectedComponent = InComponent; }

	void ChangeGameViewportClient()
	{
		FViewportClient* GameViewportClient = new FGameViewportClient();

		FViewportContext* ViewportContext = WindowManager.FindPerspectiveViewportContext();

		OldViewportClient = ViewportContext->ViewportClient;

		ViewportContext->ViewportClient->Detach();
		ViewportContext->ViewportClient = GameViewportClient;
		ViewportContext->ViewportClient->Initialize(InputManager, EnhancedInput);
		ViewportContext->ViewportClient->Attach();
	}

	void RestoreEditorViewportClient()
	{
		FViewportContext* ViewportContext = WindowManager.FindGameViewportContext();
		if (ViewportContext == nullptr)
		{
			ViewportContext = WindowManager.FindPerspectiveViewportContext();
		}
		if (OldViewportClient)
		{
			ViewportContext->ViewportClient->Detach();
			delete ViewportContext->ViewportClient;
			ViewportContext->ViewportClient = OldViewportClient;
			OldViewportClient = nullptr;
			ViewportContext->ViewportClient->Initialize(InputManager, EnhancedInput);
			ViewportContext->ViewportClient->Attach();

			// PIE 종료 후 캐시 무효화 — 피킹이 에디터 월드 UUID를 사용하도록
			ViewportContext->ViewportClient->GetRenderCollector().MarkDirty();
			extern ENGINE_API class FRenderer* GRenderer;
			if (GRenderer) GRenderer->ClearCachedBatches();
		}
	}

protected:
	void PreInitialize() override;
	void PostInitialize() override;
	void ProcessInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam) override;
	void Tick(float DeltaTime) override;
	void Render() override;
	void OnMainWindowResized(int32 Width, int32 Height) override;
	EWorldType GetStartupLevelType() const override { return EWorldType::Editor; }
	FViewportClient* CreateViewportClient() override;

private:
	bool TryRunPendingObjViewerStartupPrompt();
	void RunObjViewerStartupTest();

	AActor* SelectedActor = nullptr;
	USceneComponent* SelectedComponent = nullptr;

	FEditorUI EditorUI;
	FWindowManager WindowManager;
	bool bPendingObjViewerStartupPrompt = false;

	EPIEState PIEState = EPIEState::Stopped;

	FViewportClient* OldViewportClient = nullptr;
};

extern FEditorEngine* GEditor;
