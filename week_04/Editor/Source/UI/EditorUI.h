#pragma once
#include "OutlinerWindow.h" 
#include "ControlPanelWindow.h"
#include "PropertyWindow.h"
#include "ConsoleWindow.h"
#include "StatWindow.h"
#include "Viewport.h"
#include "Types/ObjectPtr.h"
#include "ContentBrowserWindow.h"

class FCore;
class FWindow;
class FRenderer;
class AActor;
class FEditorViewportClient;
class FEditorUI
{
public:
	void Initialize(FCore* InCore);
	void SetupWindow(FWindow* InWindow);
	void AttachToRenderer(FRenderer* InRenderer);
	void DetachFromRenderer(FRenderer* InRenderer);
	void Render();
	void SyncSelectedActorProperty();
	bool GetViewportMousePosition(int32 WindowMouseX, int32 WindowMouseY, int32& OutViewportX, int32& OutViewportY, int32& OutWidth, int32& OutHeight) const;
	bool IsViewportInteractive() const;

	FConsoleWindow& GetConsole() { return Console; }
	FCore* GetCore() { return Core; }

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
	FViewport Viewport;
	FOutlinerWindow Outliner;
	FContentBrowserWindow ContentBrowser;

	bool bWindowSetup = false;
	bool bViewportClientActive = false;
	bool bLayoutInitialized = false;
	FRenderer* CurrentRenderer = nullptr;
};
