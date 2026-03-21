#pragma once

#include "ControlPanelWindow.h"
#include "PropertyWindow.h"
#include "ConsoleWindow.h"
#include "StatWindow.h"
#include "Viewport.h"
#include "Types/ObjectPtr.h"

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
	void SyncSelectedActorProperty();
	bool GetViewportMousePosition(int32 WindowMouseX, int32 WindowMouseY, int32& OutViewportX, int32& OutViewportY, int32& OutWidth, int32& OutHeight) const;
	bool IsViewportInteractive() const;

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
	CViewport Viewport;

	bool bWindowSetup = false;
	bool bViewportClientActive = false;
	bool bLayoutInitialized = false;
	CRenderer* CurrentRenderer = nullptr;
};
