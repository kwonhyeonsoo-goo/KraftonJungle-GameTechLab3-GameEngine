#include "EditorGUI.h"
#include "Renderer/Renderer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

void CEditorGUI::Initialize(CRenderer* Renderer)
{
	HWND                 Hwnd = Renderer->GetHwnd();
	ID3D11Device* Device = Renderer->GetDevice();
	ID3D11DeviceContext* DeviceContext = Renderer->GetDeviceContext();

	Renderer->SetGUICallbacks(
		[Hwnd, Device, DeviceContext]()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.IniFilename = "imgui_editor.ini";

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(0, 0);
		style.DisplayWindowPadding = ImVec2(0, 0);
		style.DisplaySafeAreaPadding = ImVec2(0, 0);
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

	Renderer->SetGUIUpdateCallback([this]() { Update(); });
}

void CEditorGUI::BuildDefaultLayout(unsigned int DockID)
{
	ImGui::DockBuilderRemoveNode(DockID);
	ImGui::DockBuilderAddNode(DockID, ImGuiDockNodeFlags_DockSpace);

	ImGuiViewport* Viewport = ImGui::GetMainViewport();
	ImGui::DockBuilderSetNodeSize(DockID, Viewport->WorkSize);

	// 1. 하단 먼저 — 전체 폭에서 분리
	ImGuiID DockBottom, DockUpper;
	ImGui::DockBuilderSplitNode(DockID, ImGuiDir_Down, 0.25f, &DockBottom, &DockUpper);

	// 2. 상단 영역에서 좌측 분리
	ImGuiID DockLeft, DockCenter;
	ImGui::DockBuilderSplitNode(DockUpper, ImGuiDir_Left, 0.20f, &DockLeft, &DockCenter);

	// 3. 나머지 중앙에서 우측 분리
	ImGuiID DockRight;
	ImGui::DockBuilderSplitNode(DockCenter, ImGuiDir_Right, 0.25f, &DockRight, &DockCenter);

	ImGui::DockBuilderDockWindow("Stats", DockLeft);
	ImGui::DockBuilderDockWindow("Properties", DockRight);
	ImGui::DockBuilderDockWindow("Console", DockBottom);

	ImGui::DockBuilderFinish(DockID);
}
void CEditorGUI::Update()
{
	// ─── 전체화면 DockSpace 호스트 ───
	ImGuiViewport* Viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(Viewport->WorkPos);
	ImGui::SetNextWindowSize(Viewport->WorkSize);
	ImGui::SetNextWindowViewport(Viewport->ID);

	ImGuiWindowFlags HostFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::Begin("##DockSpaceHost", nullptr, HostFlags);
	ImGui::PopStyleVar(3);
	ImGuiID DockID = ImGui::GetID("MainDockSpace");

	// ini 파일이 없을 때 최초 1회만 레이아웃 빌드
	if (!bLayoutInitialized)
	{
		bLayoutInitialized = true;


		ImGuiDockNode* Node = ImGui::DockBuilderGetNode(DockID);
		if (!Node || Node->IsEmpty())
		{
			BuildDefaultLayout(DockID);
		}
	}
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::DockSpace(DockID, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::PopStyleVar();
	ImGui::End();

	// ─── 패널 렌더 ───
	Stats.Render();
	Property.Render();
	Console.Render();
}