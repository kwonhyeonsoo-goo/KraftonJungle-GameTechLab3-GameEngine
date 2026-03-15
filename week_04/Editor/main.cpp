#include "EngineTest.h"
#include "Platform/Windows/WindowApplication.h"
#include "Platform/Windows/Window.h"
#include "Core/Core.h"
#include "Renderer/Renderer.h"
#include "Debug/EngineLog.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "UI/EditorUI.h"

static CEditorUI GEditorUI;

int EngineMain(HINSTANCE hInstance)
{
	ImGui_ImplWin32_EnableDpiAwareness();

	// Application & Window
	CWindowApplication& App = CWindowApplication::Get();
	if (!App.Create(hInstance))
		return -1;

	CWindow* MainWindow = App.MakeWindow(L"Jungle Editor", 1280, 720);
	if (!MainWindow)
		return -1;

	// Core
	CCore Core;
	if (!Core.Initialize(MainWindow->GetHwnd(), MainWindow->GetWidth(), MainWindow->GetHeight()))
		return -1;

	// Editor UI (ImGui init, message filters, picking, resize, panels)
	GEditorUI.Initialize(&Core);
	GEditorUI.SetupWindow(MainWindow);

	// UE_LOG
	FEngineLog::Get().SetCallback([](const char* Msg)
	{
		GEditorUI.GetConsole().AddLog("%s", Msg);
	});
	UE_LOG("Engine initialized");

	// Main loop
	while (App.PumpMessages())
	{
		if (Core.GetRenderer()->IsOccluded())
			continue;

		Core.Tick();
	}

	// Cleanup
	Core.Release();
	delete MainWindow;
	App.Destroy();

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	return EngineMain(hInstance);
}
