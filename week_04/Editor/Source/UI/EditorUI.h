#pragma once
#include "OutlinerWindow.h"
#include "ControlPanelWindow.h"
#include "PropertyWindow.h"
#include "ConsoleWindow.h"
#include "StatWindow.h"
#include "Viewport.h"
#include "Types/ObjectPtr.h"
#include "ContentBrowserWindow.h"
#include <vector>

class FCore;
class FWindow;
class FRenderer;
class AActor;
class IViewportClient;

class FEditorUI
{
public:
	void Initialize(FCore* InCore);
	void SetupWindow(FWindow* InWindow);
	void AttachToRenderer(FRenderer* InRenderer);
	void DetachFromRenderer(FRenderer* InRenderer);
	void Render();
	void SyncSelectedActorProperty();

	// 포커스된 뷰포트 기준 마우스 위치 반환
	bool GetViewportMousePosition(int32 WindowMouseX, int32 WindowMouseY,
		int32& OutViewportX, int32& OutViewportY,
		int32& OutWidth, int32& OutHeight) const;
	bool IsViewportInteractive() const;

	FConsoleWindow& GetConsole() { return Console; }
	FCore* GetCore() { return Core; }

	// FViewport[Index] ↔ IViewportClient 1:1 연결
	// FEditorEngine::PostInitialize()에서 호출
	void LinkViewportClient(int32 Index, IViewportClient* InClient);

	// 포커스/호버된 FViewport에 연결된 ViewportClient 반환
	// 없으면 Viewports[0]의 ViewportClient 반환
	IViewportClient* GetFocusedViewportClient() const;

	// Index 0 기준 EditorViewportClient 접근용
	IViewportClient* GetPrimaryViewportClient() const;

private:
	void BuildDefaultLayout(uint32 DockID);
	void LoadEditorSettings();
	void SaveEditorSettings();
	std::wstring GetEditorIniPathW() const;

	FCore* Core = nullptr;
	TObjectPtr<AActor> CachedSelectedActor;
	FWindow* MainWindow = nullptr;

	FControlPanelWindow ControlPanel;
	FPropertyWindow Property;
	FConsoleWindow Console;
	FStatWindow Stat;
	FOutlinerWindow Outliner;
	FContentBrowserWindow ContentBrowser;

	// 다중 뷰포트 — ViewportClientArray와 인덱스 1:1 대응
	std::vector<FViewport> Viewports;

	bool bWindowSetup = false;
	bool bViewportActive = false;
	bool bLayoutInitialized = false;
	FRenderer* CurrentRenderer = nullptr;
};