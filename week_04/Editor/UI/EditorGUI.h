// EditorGUI.h
#include "ConsoleWindow.h"
#include "PropertyWindow.h"
#include "StatWindow.h"
#include "imgui.h"
class CRenderer;

class CEditorGUI
{
public:
	void Initialize(CRenderer* Renderer);
	void Update();

	CConsoleWindow& GetConsole() { return Console; }
	CPropertyWindow& GetProperty() { return Property; }
	CStatWindow& GetStats() { return Stats; }

private:
	void BuildDefaultLayout(ImGuiID DockID);

	CConsoleWindow  Console;
	CPropertyWindow Property;
	CStatWindow     Stats;

	bool bLayoutInitialized = false;
};
