#include "EditorUI.h"
#include "EditorControlPanel.h"
#include "EditorConsolePanel.h"
#include "EditorPropertyPanel.h"

#include "Core/Core.h"

#include "Thirdparty/ImGui/imgui.h"
#include "Thirdparty/ImGui/imgui_impl_win32.h"
#include "Thirdparty/ImGui/imgui_impl_dx11.h"

FEditorUI::FEditorUI(FCore* InCore)
{
	Core = InCore;
}

FEditorUI::~FEditorUI()
{
	Shutdown();
}

void FEditorUI::Initialize(HWND HWnd, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init(HWnd);
	ImGui_ImplDX11_Init(Device, DeviceContext);

	ControlPanel = std::make_unique<FEditorControlPanel>(Core);
	ConsolePanel = std::make_unique<FEditorConsolePanel>(Core);
	PropertyPanel = std::make_unique<FEditorPropertyPanel>(Core);
}

void FEditorUI::Shutdown()
{
	if (PropertyPanel) PropertyPanel.reset();
	if (ConsolePanel) ConsolePanel.reset();
	if (ControlPanel) ControlPanel.reset();
}

void FEditorUI::Render()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ControlPanel->Render();
	ConsolePanel->Render();
	PropertyPanel->Render();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
