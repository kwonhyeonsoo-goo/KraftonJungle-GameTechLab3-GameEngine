#pragma once
#include "OutlinerWindow.h"
#include "ControlPanelWindow.h"
#include "PropertyWindow.h"
#include "ConsoleWindow.h"
#include "ObjViewerPanel.h"
#include "StatWindow.h"
#include "Viewport.h"
#include "Math/Rect.h"
#include "Types/ObjectPtr.h"
#include "ContentBrowserWindow.h"

class FCore;
class FWindow;
class FRenderer;
class AActor;
class FEditorViewportClient;
class FWindowManager;
class FCamera;

class FEditorUI
{
public:
	void Initialize(FCore* InCore, class FWindowManager* InWindowManager);
	void SetupWindow(FWindow* InWindow);
	void AttachToRenderer();
	void DetachFromRenderer();
	void Render();
	void SyncSelectedActorProperty();
	void SaveEditorSettings();
	void SetActiveViewportClient(FEditorViewportClient* InViewportClient) { ActiveViewportClient = InViewportClient; }
	FEditorViewportClient* GetActiveViewportClient() const { return ActiveViewportClient; }
	bool HasActiveViewportClient() const { return ActiveViewportClient != nullptr; }
	FEditorViewportClient* FindPerspectiveViewportClient() const;
	FCamera* GetPerspectiveCamera() const;
	FRect GetCentralDockSpaceRect() const;
	void SavePerspectiveCameraInitialState() const;
	//bool GetViewportMousePosition(int32 WindowMouseX, int32 WindowMouseY, int32& OutViewportX, int32& OutViewportY, int32& OutWidth, int32& OutHeight) const;
	//bool IsViewportInteractive() const;

	FConsoleWindow& GetConsole() { return Console; }
	FCore* GetCore() { return Core; }

private:
	void BuildDefaultLayout(uint32 DockID);
	void LoadEditorSettings();
	std::wstring GetEditorIniPathW() const;

	FCore* Core = nullptr;
	TObjectPtr<AActor> CachedSelectedActor;
	FWindow* MainWindow = nullptr;

	FControlPanelWindow ControlPanel;
	FPropertyWindow Property;
	FConsoleWindow Console;
	FObjViewerPanel ObjViewerPanel;
	FStatWindow Stat;
	FViewportLegacy ViewportLegacy;
	FOutlinerWindow Outliner;
	FContentBrowserWindow ContentBrowser;

	bool bWindowSetup = false;
	bool bViewportClientActive = false;
	bool bLayoutInitialized = false;
	bool bConsumeConsoleShortcutChar = false;
	FRect CachedCentralDockSpaceRect;
	FRenderer* CurrentRenderer = nullptr;
	FEditorViewportClient* ActiveViewportClient = nullptr;
	class FWindowManager* WindowManager = nullptr;
};
