
#include "ControlPanelWindow.h"
#include "ConsoleWindow.h"
#include "PropertyWindow.h"
#include "StatWindow.h"
#include "imgui.h"
class CRenderer;
class CCore;
class AActor;


class CEditorGUI
{
public:
	void Initialize(CRenderer* Renderer, CCore* Core);
	void Update();

	CConsoleWindow& GetConsole() { return Console; }
	CPropertyWindow& GetProperty() { return Property; }
	CStatWindow& GetStats() { return Stats; }
	CControlPanelWindow ControlPanel;
private:
	void BuildDefaultLayout(unsigned int DockID);

	CConsoleWindow  Console;
	CPropertyWindow Property;
	CStatWindow     Stats;

	bool bLayoutInitialized = false;
	CCore* EngineCore = nullptr; 
	AActor* CachedSelectedActor = nullptr;
};