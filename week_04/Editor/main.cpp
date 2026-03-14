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

static void SetupImGuiCallbacks(CRenderer* Renderer)
{
	HWND Hwnd = Renderer->GetHwnd();
	ID3D11Device* Device = Renderer->GetDevice();
	ID3D11DeviceContext* DeviceContext = Renderer->GetDeviceContext();

	Renderer->SetGUICallbacks(
		[Hwnd, Device, DeviceContext]()
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

			ImGui::StyleColorsDark();

			ImGuiStyle& style = ImGui::GetStyle();
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				style.WindowRounding = 0.0f;
				style.Colors[ImGuiCol_WindowBg].w = 1.0f;
			}

			ImGui_ImplWin32_Init(Hwnd);
			ImGui_ImplDX11_Init(Device, DeviceContext);
		},
		[]()
		{
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		},
		[]()
		{
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
		},
		[]()
		{
			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		},
		[]()
		{
			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
		}
	);
}

int EngineMain(HINSTANCE hInstance)
{
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

	// ImGui
	SetupImGuiCallbacks(Core.GetRenderer());

	// Editor UI (message filters, picking, resize, panels)
	GEditorUI.Initialize(&Core);
	GEditorUI.SetupWindow(MainWindow);

	// UE_LOG
	FEngineLog::Get().SetCallback([](const char* Msg)
	{
		GEditorUI.GetConsole().AddLog("%s", Msg);
	});
	UE_LOG("Engine initialized");

	// GUI update
	Core.GetRenderer()->SetGUIUpdateCallback([&]()
	{
		GEditorUI.Render();
	});

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
