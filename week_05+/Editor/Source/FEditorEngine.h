#pragma once

#include "PIEState.h"
#include "Core/FEngine.h"
#include "Core/GameViewportClient.h"
#include "UI/EditorViewportClient.h"
#include "UI/EditorUI.h"
#include "UI/WindowManager.h"

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
	void SetSelectedActor(AActor* InActor) { SelectedActor = InActor; EditorUI.SyncSelectedActorProperty(); }

	void ChangeGameViewportClient()
	{
		FViewportClient* GameViewportClient = new FGameViewportClient();

		FViewportContext* ViewportContext = WindowManager.FindPerspectiveViewportContext();
		if (ViewportContext == nullptr)
		{
			ViewportContext = WindowManager.FindViewportContext();
		}

		OldViewportClient = ViewportContext->ViewportClient;

		ViewportContext->ViewportClient->Detach();
		ViewportContext->ViewportClient = GameViewportClient;
		ViewportContext->ViewportClient->Initialize(InputManager, EnhancedInput);
		ViewportContext->ViewportClient->Attach();
	}

	void RestoreEditorViewportClient()
	{
		FViewportContext* ViewportContext = WindowManager.FindPerspectiveViewportContext();
		if (ViewportContext == nullptr)
		{
			ViewportContext = WindowManager.FindViewportContext();
		}
		if (OldViewportClient)
		{
			ViewportContext->ViewportClient->Detach();
			delete ViewportContext->ViewportClient;
			ViewportContext->ViewportClient = OldViewportClient;
			OldViewportClient = nullptr;
			ViewportContext->ViewportClient->Initialize(InputManager, EnhancedInput);
			ViewportContext->ViewportClient->Attach();
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

	FEditorUI EditorUI;
	FWindowManager WindowManager;
	bool bPendingObjViewerStartupPrompt = false;

	EPIEState PIEState = EPIEState::Stopped;

	FViewportClient* OldViewportClient = nullptr;
};

extern FEditorEngine* GEditor;
