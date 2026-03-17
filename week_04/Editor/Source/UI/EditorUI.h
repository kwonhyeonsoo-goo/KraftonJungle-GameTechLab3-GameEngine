#pragma once

#include "ControlPanelWindow.h"
#include "PropertyWindow.h"
#include "ConsoleWindow.h"
#include "StatWindow.h"

class CCore;
class CWindow;
class CRenderer;
class AActor;

class CEditorUI
{
public:
	void Initialize(CCore* InCore);
	void SetupWindow(CWindow* InWindow);
	void AttachToRenderer(CRenderer* InRenderer);
	void DetachFromRenderer(CRenderer* InRenderer);
	void Render();

	CConsoleWindow& GetConsole() { return Console; }

private:
	void BuildDefaultLayout(uint32 DockID);

	CCore* Core = nullptr;
	TObjectPtr<AActor> CachedSelectedActor;

	CWindow* MainWindow = nullptr;

	CControlPanelWindow ControlPanel;
	CPropertyWindow Property;
	CConsoleWindow Console;
	CStatWindow Stat;

	bool bWindowSetup = false;
	bool bViewportClientActive = false;
	bool bLayoutInitialized = false;
};
