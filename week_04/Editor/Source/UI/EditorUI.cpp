#include "EditorUI.h"

#include "Core/Core.h"
#include "Object/Object.h"
#include "World/Level.h"
#include "Actor/Actor.h"
#include "Component/PrimitiveComponent.h"
#include "Component/SceneComponent.h"
#include "Component/CameraComponent.h"
#include "Platform/Windows/Window.h"
#include "Renderer/Renderer.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "Core/ViewportClient.h"
#include "Core/Paths.h"

#include <windows.h>
#include <commdlg.h>

#include "UI/EditorViewportClient.h"
#include "Debug/EngineLog.h"
#include "Serializer/SceneSerializer.h"
#include "Actor/SkySphereActor.h"
#include "Actor/ObjActor.h"
#include "Core/ShowFlags.h"
#include "EditorViewportClient.h"

enum class EFileDialogType { Open, Save };

std::string GetFilePathUsingDialog(EFileDialogType Type)
{
	char FileName[MAX_PATH] = "";
	FString ContentDir = FPaths::ContentDir().string();

	OPENFILENAMEA Ofn = {};
	Ofn.lStructSize = sizeof(OPENFILENAMEA);
	Ofn.lpstrFilter = "Level Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
	Ofn.lpstrFile = FileName;
	Ofn.nMaxFile = MAX_PATH;
	Ofn.lpstrDefExt = "json";
	Ofn.lpstrInitialDir = ContentDir.c_str();

	if (Type == EFileDialogType::Save)
	{
		Ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
		if (GetSaveFileNameA(&Ofn)) return std::string(FileName);
	}
	else
	{
		Ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		if (GetOpenFileNameA(&Ofn)) return std::string(FileName);
	}
	return "";
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// ── LinkViewportClient / 접근자 ────────────────────────────────────────────

void FEditorUI::LinkViewportClient(int32 Index, IViewportClient* InClient)
{
	// 배열 크기 자동 확장
	if (Index >= static_cast<int32>(Viewports.size()))
	{
		Viewports.resize(Index + 1);
	}
	Viewports[Index].SetLinkedViewportClient(InClient);
}

IViewportClient* FEditorUI::GetFocusedViewportClient() const
{
	for (const FViewport& VP : Viewports)
	{
		if (VP.IsFocused() || VP.IsHovered())
		{
			return VP.GetLinkedViewportClient();
		}
	}
	return GetPrimaryViewportClient();
}

IViewportClient* FEditorUI::GetPrimaryViewportClient() const
{
	if (!Viewports.empty())
	{
		return Viewports[0].GetLinkedViewportClient();
	}
	return nullptr;
}

// ── Initialize ─────────────────────────────────────────────────────────────

void FEditorUI::Initialize(FCore* InCore)
{
	Core = InCore;

	Property.OnChanged = [this](const FVector& Loc, const FVector& Rot, const FVector& Scl)
		{
			if (!Core) return;
			AActor* Selected = Core->GetSelectedActor();
			if (!Selected) return;
			if (USceneComponent* Root = Selected->GetRootComponent())
			{
				FTransform Transform = Root->GetRelativeTransform();
				Transform.SetLocation(Loc);
				Transform.SetRotation(FRotator::MakeFromEuler(Rot));
				Transform.SetScale3D(Scl);
				Root->SetRelativeTransform(Transform);
			}
		};

	ContentBrowser.OnFileDoubleClickCallback = [this](const FString& FilePath)
		{
			if (IViewportClient* VP = GetFocusedViewportClient())
			{
				VP->HandleFileDoubleClick(FilePath);
			}
		};

	ContentBrowser.OnFileDragEnd = [this](const FString& DraggingFilePath, const FString& ReleaseDirectory)
		{
			if (ContentBrowser.IsHovered())
			{
				if (ContentBrowser.IsMouseOnDirectory())
				{
					std::filesystem::path Src = DraggingFilePath;
					std::filesystem::path Dst = std::filesystem::path(ReleaseDirectory) / Src.filename();

					std::error_code ec;
					if (std::filesystem::exists(Dst))
					{
						int Result = MessageBoxW(nullptr,
							L"이미 같은 이름의 파일이 존재합니다.\n덮어쓰시겠습니까?",
							L"Overwrite", MB_YESNO | MB_ICONWARNING);
						if (Result != IDYES) return;
						std::filesystem::remove(Dst, ec);
						if (ec) { MessageBoxW(nullptr, L"Delete Failed", L"Error", MB_OK | MB_ICONERROR); return; }
					}
					std::filesystem::rename(Src, Dst, ec);
					if (ec) UE_LOG("Move Failed: %s", ec.message().c_str());
					else    UE_LOG("Moved: %s -> %s", Src.string().c_str(), Dst.string().c_str());
				}
			}
			else
			{
				// 포커스된 뷰포트에 드롭
				for (const FViewport& VP : Viewports)
				{
					if (VP.IsHovered())
					{
						if (IViewportClient* Client = VP.GetLinkedViewportClient())
						{
							UE_LOG("Drop On Viewport");
							Client->HandleFileDropOnViewport(DraggingFilePath);
						}
						break;
					}
				}
			}
		};
}

// ── AttachToRenderer ────────────────────────────────────────────────────────

void FEditorUI::AttachToRenderer(FRenderer* InRenderer)
{
	if (!Core || !InRenderer) return;

	bViewportActive = true;
	CurrentRenderer = InRenderer;

	const HWND Hwnd = InRenderer->GetHwnd();
	ID3D11Device* Device = InRenderer->GetDevice();
	ID3D11DeviceContext* DC = InRenderer->GetDeviceContext();

	ContentBrowser.SetFolderIcon(CurrentRenderer->GetFolderIconSRV());
	ContentBrowser.SetFileIcon(CurrentRenderer->GetFileIconSRV());

	std::filesystem::path FontPath = FPaths::ProjectRoot() / "Content" / "Fonts" / "NotoSansKR-Bold.ttf";
	std::wstring FontPathW = FontPath.wstring();

	InRenderer->SetGUICallbacks(
		[Hwnd, Device, DC, FontPathW, FontPath]()
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& IO = ImGui::GetIO();
			IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
			IO.IniFilename = "imgui_editor.ini";

			ImFontConfig FontConfig;
			FontConfig.OversampleH = 1;
			FontConfig.OversampleV = 1;
			FontConfig.PixelSnapH = true;

			FILE* f = nullptr;
			_wfopen_s(&f, FontPath.c_str(), L"rb");
			if (f)
			{
				fseek(f, 0, SEEK_END);
				size_t Size = ftell(f);
				fseek(f, 0, SEEK_SET);
				void* Data = IM_ALLOC(Size);
				fread(Data, 1, Size, f);
				fclose(f);
				IO.Fonts->AddFontFromMemoryTTF(Data, (int)Size, 16.f, &FontConfig, IO.Fonts->GetGlyphRangesKorean());
			}
			else
			{
				IO.Fonts->AddFontDefault();
				MessageBoxW(nullptr, FontPathW.c_str(), L"Failed to load font", MB_OK);
			}

			ImGui::StyleColorsDark();
			ImGuiStyle& Style = ImGui::GetStyle();
			Style.WindowPadding = ImVec2(0, 0);
			Style.DisplayWindowPadding = ImVec2(0, 0);
			Style.DisplaySafeAreaPadding = ImVec2(0, 0);
			Style.Colors[ImGuiCol_Text] = ImVec4(1.f, 1.f, 1.f, 1.f);
			Style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.6f, 0.6f, 0.6f, 1.f);
			if (IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				Style.WindowRounding = 0.f;
				Style.Colors[ImGuiCol_WindowBg].w = 1.f;
			}
			ImGui_ImplWin32_Init(Hwnd);
			ImGui_ImplDX11_Init(Device, DC);
		},
		[]() { ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext(); },
		[]() { ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame(); },
		[]() { ImGui::Render(); ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); },
		[]()
		{
			ImGuiIO& IO = ImGui::GetIO();
			if (IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
		}
	);

	InRenderer->SetGUIUpdateCallback([this]() { Render(); });

	InRenderer->SetPostRenderCallback([this](FRenderer* Renderer)
		{
			if (!Core) return;
			AActor* Selected = Core->GetSelectedActor();

			IViewportClient* PrimaryVP = GetPrimaryViewportClient();
			if (!PrimaryVP) return;

			if (Selected && !Selected->IsPendingDestroy() && Selected->IsVisible()
				&& !Selected->IsA<ASkySphereActor>()
				&& PrimaryVP->GetShowFlags().HasFlag(EEngineShowFlags::SF_Primitives))
			{
				for (UActorComponent* Component : Selected->GetComponents())
				{
					if (!Component->IsA(UPrimitiveComponent::StaticClass())) continue;
					UPrimitiveComponent* PC = static_cast<UPrimitiveComponent*>(Component);
					if (PC->GetPrimitive())
					{
						Renderer->RenderOutline(PC->GetPrimitive()->GetMeshData(), PC->GetWorldTransform());
					}
				}
			}
		});

	LoadEditorSettings();
}

// ── DetachFromRenderer ──────────────────────────────────────────────────────

void FEditorUI::DetachFromRenderer(FRenderer* InRenderer)
{
	bViewportActive = false;
	CurrentRenderer = nullptr;

	for (FViewport& VP : Viewports)
	{
		VP.ReleaseLevelView();
	}

	if (InRenderer)
	{
		InRenderer->ClearLevelRenderTarget();
		InRenderer->ClearViewportCallbacks();
	}
}

// ── SetupWindow ─────────────────────────────────────────────────────────────

void FEditorUI::SetupWindow(FWindow* InWindow)
{
	MainWindow = InWindow;
	if (bWindowSetup || !MainWindow) return;
	bWindowSetup = true;

	MainWindow->AddMessageFilter([this](HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam) -> bool
		{
			if (!bViewportActive) return false;

			const bool bIsIme =
				Msg == WM_IME_STARTCOMPOSITION || Msg == WM_IME_COMPOSITION ||
				Msg == WM_IME_ENDCOMPOSITION || Msg == WM_IME_NOTIFY ||
				Msg == WM_IME_SETCONTEXT || Msg == WM_IME_CHAR;
			const bool bIsChar = Msg == WM_CHAR || Msg == WM_SYSCHAR || Msg == WM_UNICHAR;

			if (bIsIme || bIsChar)
			{
				if (ImGui::GetCurrentContext() && !ImGui::GetIO().WantTextInput)
					return true;
				else if (!ImGui::GetCurrentContext())
					return true;
			}

			const bool bHandledByImGui = ImGui_ImplWin32_WndProcHandler(Hwnd, Msg, WParam, LParam) != 0;
			if (IsViewportInteractive()) return false;
			return bHandledByImGui;
		});
}

// ── Render ──────────────────────────────────────────────────────────────────

void FEditorUI::Render()
{
	static bool bOpenAboutPopup = false;

	if (!bViewportActive) return;

	ImGuiViewport* MainVP = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(MainVP->WorkPos);
	ImGui::SetNextWindowSize(MainVP->WorkSize);
	ImGui::SetNextWindowViewport(MainVP->ID);

	ImGuiWindowFlags HostFlags =
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	ImGui::Begin("##DockSpaceHost", nullptr, HostFlags);
	ImGui::PopStyleVar(3);

	ImGuiID DockID = ImGui::GetID("MainDockSpace");
	if (!bLayoutInitialized)
	{
		bLayoutInitialized = true;
		ImGuiDockNode* Node = ImGui::DockBuilderGetNode(DockID);
		if (!Node || Node->IsEmpty()) BuildDefaultLayout(DockID);
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::DockSpace(DockID, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::PopStyleVar();
	ImGui::End();

	// ── 뷰포트 배열 렌더 ────────────────────────────────────────────────
	const HWND Hwnd = MainWindow ? MainWindow->GetHwnd() : nullptr;
	for (FViewport& VP : Viewports)
	{
		VP.Render(Core, CurrentRenderer, Hwnd);
	}

	// ── Actor 선택 동기화 ────────────────────────────────────────────────
	if (Core)
	{
		AActor* Selected = Core->GetSelectedActor();
		if (Selected != CachedSelectedActor) SyncSelectedActorProperty();

		const FTimer& Timer = Core->GetTimer();
		Stat.SetFPS(Timer.GetDisplayFPS());
		Stat.SetFrameTimeMs(Timer.GetFrameTimeMs());
	}

	Stat.SetObjectCount(UObject::TotalAllocationCounts);
	Stat.SetHeapUsage(UObject::TotalAllocationBytes);

	// ── 메인 메뉴바 ─────────────────────────────────────────────────────
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Level"))
			{
				if (Core)
				{
					Core->SetSelectedActor(nullptr);
					// UCameraComponent 직접 접근 — FCamera 제거됨
					if (IViewportClient* VP = GetPrimaryViewportClient())
					{
						UCameraComponent* Cam = VP->GetActiveCamera();
						if (Cam)
						{
							Cam->SetPosition({ -5.f, 0.f, 2.f });
							Cam->SetRotation(0.f, 0.f);
						}
					}
					Core->GetLevel()->ClearActors();
					UE_LOG("New Level created");
				}
			}

			if (ImGui::MenuItem("Open Level"))
			{
				if (Core && Core->GetActiveLevel())
				{
					FString Path = GetFilePathUsingDialog(EFileDialogType::Open);
					if (!Path.empty())
					{
						Core->SetSelectedActor(nullptr);
						Core->GetLevel()->ClearActors();

						UCameraComponent* Cam = nullptr;
						if (IViewportClient* VP = GetPrimaryViewportClient())
							Cam = VP->GetActiveCamera();

						bool bLoaded = FSceneSerializer::Load(
							Core->GetLevel(), Path,
							Core->GetRenderer()->GetDevice(), Cam);

						if (bLoaded) UE_LOG("Level loaded: %s", Path.c_str());
						else MessageBoxW(nullptr, L"Level 정보가 잘못되었습니다.", L"Error", MB_OK | MB_ICONWARNING);
					}
				}
			}

			if (ImGui::MenuItem("Save Level As..."))
			{
				if (Core && Core->GetActiveLevel())
				{
					FString Path = GetFilePathUsingDialog(EFileDialogType::Save);
					if (!Path.empty())
					{
						UCameraComponent* Cam = nullptr;
						if (IViewportClient* VP = GetPrimaryViewportClient())
							Cam = VP->GetActiveCamera();

						FSceneSerializer::Save(Core->GetLevel(), Path, Cam);
					}
				}
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			// 포커스된 뷰포트의 ViewportClient 기준으로 ShowFlags 편집
			IViewportClient* FocusedVP = GetFocusedViewportClient();
			FEditorViewportClient* EditorVP =
				FocusedVP ? dynamic_cast<FEditorViewportClient*>(FocusedVP) : nullptr;

			if (EditorVP)
			{
				FShowFlags& ShowFlags = EditorVP->GetShowFlags();
				ImGui::SeparatorText("Show Flags");

				auto ShowFlagCheckbox = [&](const char* Label, EEngineShowFlags Flag)
					{
						bool bValue = ShowFlags.HasFlag(Flag);
						if (ImGui::Checkbox(Label, &bValue))
						{
							ShowFlags.SetFlag(Flag, bValue);
							SaveEditorSettings();
						}
					};

				ShowFlagCheckbox("Primitives", EEngineShowFlags::SF_Primitives);
				ShowFlagCheckbox("UUID", EEngineShowFlags::SF_UUID);
				ShowFlagCheckbox("Debug Draw", EEngineShowFlags::SF_DebugDraw);
				ShowFlagCheckbox("Collision", EEngineShowFlags::SF_Collision);

				ImGui::SeparatorText("Grid");
				bool bShowGrid = EditorVP->IsGridVisible();
				if (ImGui::Checkbox("Show Grid", &bShowGrid))
				{
					EditorVP->SetGridVisible(bShowGrid);
					SaveEditorSettings();
				}
				float GridSize = EditorVP->GetGridSize();
				if (ImGui::SliderFloat("Grid Size", &GridSize, 1.f, 100.f, "%.1f"))
				{
					EditorVP->SetGridSize(GridSize);
					SaveEditorSettings();
				}
				float Thickness = EditorVP->GetLineThickness();
				if (ImGui::SliderFloat("Line Thickness", &Thickness, 0.1f, 5.f, "%.2f"))
				{
					EditorVP->SetLineThickness(Thickness);
					SaveEditorSettings();
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("About")) bOpenAboutPopup = true;
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	// ── About 팝업 ───────────────────────────────────────────────────────
	if (bOpenAboutPopup) { ImGui::OpenPopup("AboutPopup"); ImGui::SetNextWindowSize(ImVec2(420, 320), ImGuiCond_Always); bOpenAboutPopup = false; }
	if (ImGui::BeginPopupModal("AboutPopup", nullptr, ImGuiWindowFlags_NoTitleBar))
	{
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		ImVec2 WPos = ImGui::GetWindowPos();
		ImVec2 WSize = ImGui::GetWindowSize();
		DrawList->AddRectFilled(WPos, ImVec2(WPos.x + WSize.x, WPos.y + 60), IM_COL32(30, 30, 60, 255));

		ImGui::SetCursorPosY(12); ImGui::SetCursorPosX((WSize.x - ImGui::CalcTextSize("Dino Engine").x) * 0.5f);
		ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.f, 1.f), "Dino Engine");
		ImGui::SetCursorPosY(35); ImGui::SetCursorPosX((WSize.x - ImGui::CalcTextSize("v1.0.0").x) * 0.5f);
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f), "v1.0.0");
		ImGui::SetCursorPosY(70); ImGui::SetCursorPosX(20);
		ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.f), "Contributors");
		ImGui::SameLine(); ImGui::SetCursorPosX(20);
		ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.9f, 0.7f, 0.3f, 0.5f)); ImGui::Separator(); ImGui::PopStyleColor();
		ImGui::Spacing();
		const char* Contributors[] = { "김지수", "김태현", "박세영", "조상현" };
		for (const char* Name : Contributors)
		{
			ImGui::SetCursorPosX(20);
			ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.6f, 1.f), "•"); ImGui::SameLine();
			ImGui::Text("%s", Name);
		}
		ImGui::Spacing(); ImGui::SetCursorPosX(20);
		ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1, 1, 1, 0.1f)); ImGui::Separator(); ImGui::PopStyleColor();
		ImGui::Spacing(); ImGui::SetCursorPosX(20);
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f), "Copyright (c) 2026  |  MIT License");
		ImGui::Spacing(); ImGui::Spacing();
		float BtnW = 100.f;
		ImGui::SetCursorPosX((WSize.x - BtnW) * 0.5f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 1.f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.7f, 1.f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
		if (ImGui::Button("Close", ImVec2(BtnW, 28))) ImGui::CloseCurrentPopup();
		ImGui::PopStyleVar(); ImGui::PopStyleColor(3);
		ImGui::Spacing(); ImGui::EndPopup();
	}

	ControlPanel.Render(Core);
	Property.Render(Core);
	Console.Render();
	Stat.Render();
	Outliner.Render(Core);
	ContentBrowser.Render();
}

// ── GetViewportMousePosition ────────────────────────────────────────────────

bool FEditorUI::GetViewportMousePosition(int32 WindowMouseX, int32 WindowMouseY,
	int32& OutViewportX, int32& OutViewportY,
	int32& OutWidth, int32& OutHeight) const
{
	// 포커스/호버된 뷰포트 우선, 없으면 첫 번째
	for (const FViewport& VP : Viewports)
	{
		if (VP.IsHovered() || VP.IsFocused())
		{
			return VP.GetMousePositionInViewport(
				WindowMouseX, WindowMouseY,
				OutViewportX, OutViewportY, OutWidth, OutHeight);
		}
	}
	if (!Viewports.empty())
	{
		return Viewports[0].GetMousePositionInViewport(
			WindowMouseX, WindowMouseY,
			OutViewportX, OutViewportY, OutWidth, OutHeight);
	}
	return false;
}

bool FEditorUI::IsViewportInteractive() const
{
	for (const FViewport& VP : Viewports)
	{
		if (VP.IsVisible() && (VP.IsHovered() || VP.IsFocused()))
			return true;
	}
	return false;
}

// ── SyncSelectedActorProperty ───────────────────────────────────────────────

void FEditorUI::SyncSelectedActorProperty()
{
	if (!Core) return;
	AActor* Selected = Core->GetSelectedActor();
	if (Selected)
	{
		if (USceneComponent* Root = Selected->GetRootComponent())
		{
			const FTransform Transform = Root->GetRelativeTransform();
			Property.SetTarget(
				Transform.GetLocation(),
				Transform.Rotator().Euler(),
				Transform.GetScale3D(),
				Selected->GetName().c_str());
		}
	}
	else
	{
		Property.SetTarget({ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, "None");
	}
	CachedSelectedActor = Selected;
}

// ── BuildDefaultLayout ──────────────────────────────────────────────────────

void FEditorUI::BuildDefaultLayout(uint32 DockID)
{
	ImGui::DockBuilderRemoveNode(DockID);
	ImGui::DockBuilderAddNode(DockID, ImGuiDockNodeFlags_DockSpace);
	ImGuiViewport* VP = ImGui::GetMainViewport();
	ImGui::DockBuilderSetNodeSize(DockID, VP->WorkSize);

	ImGuiID DockBottom = 0, DockUpper = 0;
	ImGui::DockBuilderSplitNode(DockID, ImGuiDir_Down, 0.25f, &DockBottom, &DockUpper);
	ImGuiID DockLeft = 0, DockCenter = 0;
	ImGui::DockBuilderSplitNode(DockUpper, ImGuiDir_Left, 0.20f, &DockLeft, &DockCenter);
	ImGuiID DockRight = 0;
	ImGui::DockBuilderSplitNode(DockCenter, ImGuiDir_Right, 0.25f, &DockRight, &DockCenter);
	ImGuiID DockRightTop = 0, DockRightBottom = 0;
	ImGui::DockBuilderSplitNode(DockRight, ImGuiDir_Up, 0.50f, &DockRightTop, &DockRightBottom);

	ImGui::DockBuilderDockWindow("Viewport", DockCenter);
	ImGui::DockBuilderDockWindow("Stats", DockLeft);
	ImGui::DockBuilderDockWindow("Properties", DockRightTop);
	ImGui::DockBuilderDockWindow("Control Panel", DockRightBottom);
	ImGui::DockBuilderDockWindow("Console", DockBottom);
	ImGui::DockBuilderFinish(DockID);
}

// ── LoadEditorSettings / SaveEditorSettings ─────────────────────────────────

void FEditorUI::LoadEditorSettings()
{
	std::wstring Path = GetEditorIniPathW();
	wchar_t Buf[64];

	GetPrivateProfileStringW(L"Grid", L"GridSize", L"10.0", Buf, 64, Path.c_str());
	float GridSize = static_cast<float>(_wtof(Buf));
	GetPrivateProfileStringW(L"Grid", L"LineThickness", L"1.0", Buf, 64, Path.c_str());
	float Thickness = static_cast<float>(_wtof(Buf));
	GetPrivateProfileStringW(L"Grid", L"ShowGrid", L"1", Buf, 64, Path.c_str());
	bool bShowGrid = (_wtoi(Buf) != 0);

	// 기본 EditorViewportClient(Viewports[0])에 적용
	if (FEditorViewportClient* EditorVP =
		dynamic_cast<FEditorViewportClient*>(GetPrimaryViewportClient()))
	{
		EditorVP->SetGridSize(GridSize);
		EditorVP->SetLineThickness(Thickness);
		EditorVP->SetGridVisible(bShowGrid);

		FShowFlags& SF = EditorVP->GetShowFlags();
		GetPrivateProfileStringW(L"ShowFlags", L"Primitives", L"1", Buf, 64, Path.c_str());
		SF.SetFlag(EEngineShowFlags::SF_Primitives, _wtoi(Buf) != 0);
		GetPrivateProfileStringW(L"ShowFlags", L"UUID", L"1", Buf, 64, Path.c_str());
		SF.SetFlag(EEngineShowFlags::SF_UUID, _wtoi(Buf) != 0);
		GetPrivateProfileStringW(L"ShowFlags", L"DebugDraw", L"0", Buf, 64, Path.c_str());
		SF.SetFlag(EEngineShowFlags::SF_DebugDraw, _wtoi(Buf) != 0);
		GetPrivateProfileStringW(L"ShowFlags", L"WorldAxis", L"0", Buf, 64, Path.c_str());
		SF.SetFlag(EEngineShowFlags::SF_WorldAxis, _wtoi(Buf) != 0);
		GetPrivateProfileStringW(L"ShowFlags", L"Collision", L"0", Buf, 64, Path.c_str());
		SF.SetFlag(EEngineShowFlags::SF_Collision, _wtoi(Buf) != 0);
	}
}

void FEditorUI::SaveEditorSettings()
{
	std::wstring Path = GetEditorIniPathW();
	FEditorViewportClient* EditorVP =
		dynamic_cast<FEditorViewportClient*>(GetPrimaryViewportClient());
	if (!EditorVP) return;

	wchar_t Buf[64];
	swprintf(Buf, 64, L"%.2f", EditorVP->GetGridSize());
	WritePrivateProfileStringW(L"Grid", L"GridSize", Buf, Path.c_str());
	swprintf(Buf, 64, L"%.2f", EditorVP->GetLineThickness());
	WritePrivateProfileStringW(L"Grid", L"LineThickness", Buf, Path.c_str());
	WritePrivateProfileStringW(L"Grid", L"ShowGrid", EditorVP->IsGridVisible() ? L"1" : L"0", Path.c_str());

	FShowFlags& SF = EditorVP->GetShowFlags();
	WritePrivateProfileStringW(L"ShowFlags", L"Primitives", SF.HasFlag(EEngineShowFlags::SF_Primitives) ? L"1" : L"0", Path.c_str());
	WritePrivateProfileStringW(L"ShowFlags", L"UUID", SF.HasFlag(EEngineShowFlags::SF_UUID) ? L"1" : L"0", Path.c_str());
	WritePrivateProfileStringW(L"ShowFlags", L"DebugDraw", SF.HasFlag(EEngineShowFlags::SF_DebugDraw) ? L"1" : L"0", Path.c_str());
	WritePrivateProfileStringW(L"ShowFlags", L"WorldAxis", SF.HasFlag(EEngineShowFlags::SF_WorldAxis) ? L"1" : L"0", Path.c_str());
	WritePrivateProfileStringW(L"ShowFlags", L"Collision", SF.HasFlag(EEngineShowFlags::SF_Collision) ? L"1" : L"0", Path.c_str());
}

std::wstring FEditorUI::GetEditorIniPathW() const
{
	return (FPaths::ProjectRoot() / "editor.ini").wstring();
}