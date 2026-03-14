#pragma once

#include "ControlPanelWindow.h"
#include "PropertyWindow.h"
#include "ConsoleWindow.h"
#include "StatWindow.h"
#include "Picking/Picker.h"

class CCore;
class CWindow;
class AActor;

class CEditorUI
{
public:
	void Initialize(CCore* InCore);
	void SetupWindow(CWindow* InWindow);
	void Render();

	AActor* GetSelectedActor() const { return SelectedActor; }
	void SetSelectedActor(AActor* InActor);

	CConsoleWindow& GetConsole() { return Console; }

private:
	CCore* Core = nullptr;
	AActor* SelectedActor = nullptr;

	CWindow* MainWindow = nullptr;
	CPicker Picker;

	CControlPanelWindow ControlPanel;
	CPropertyWindow Property;
	CConsoleWindow Console;
	CStatWindow Stat;
};
