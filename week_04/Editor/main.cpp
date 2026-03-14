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

	// Timing
	LARGE_INTEGER Frequency, LastTime, CurrentTime;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&LastTime);

	// Main loop
	while (App.PumpMessages())
	{
		if (Core.GetRenderer()->IsOccluded())
			continue;

		QueryPerformanceCounter(&CurrentTime);
		float DeltaTime = static_cast<float>(CurrentTime.QuadPart - LastTime.QuadPart)
			/ static_cast<float>(Frequency.QuadPart);
		LastTime = CurrentTime;

		Core.Tick(DeltaTime);
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
