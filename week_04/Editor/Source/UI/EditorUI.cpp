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

#include "Component/StaticMeshComponent.h"


#include "UI/Window/Splitter.h"


#include "Utility/FileIO.h"
#include "Object/ObjectGlobals.h"
#include "Memory/MemoryBase.h"


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
	/*if (Index >= static_cast<int32>(Viewports.size()))
	{
		Viewports.resize(Index + 1);
	}
	Viewports[Index].SetLinkedViewportClient(InClient);*/
	if (Index < 0 || Index >= (int32)Windows.size()) return;
	GetViewportAt(Index)->SetLinkedViewportClient(InClient);
}

int32 FEditorUI::GetHoveredViewportIndex() const
{
	for (int32 i = 0; i < static_cast<int32>(Windows.size()); i++)
	{
		if (Windows[i]->GetViewport()->IsHovered()) return i;
	}
	return -1;
}

bool FEditorUI::GetMousePositionInViewport(int32 ViewportIndex,
	int32 WindowMouseX, int32 WindowMouseY,
	int32& OutLocalX, int32& OutLocalY,
	int32& OutWidth, int32& OutHeight) const
{
	if (ViewportIndex < 0 || ViewportIndex >= static_cast<int32>(Windows.size()))
		return false;

	return Windows[ViewportIndex]->GetViewport()->GetMousePositionInViewport(
		WindowMouseX, WindowMouseY,
		OutLocalX, OutLocalY, OutWidth, OutHeight);
}

IViewportClient* FEditorUI::GetFocusedViewportClient() const
{
	/*for (const FViewport& VP : Viewports)
	{
		if (VP.IsFocused() || VP.IsHovered())
			return VP.GetLinkedViewportClient();
	}
	return GetPrimaryViewportClient();*/

	for (SWindow* Win : Windows)
	{
		const FViewport* VP = Win->GetViewport();
		if (VP->IsFocused() || VP->IsHovered())
		{
			return VP->GetLinkedViewportClient();
		}
	}
	return GetPrimaryViewportClient();
}

IViewportClient* FEditorUI::GetPrimaryViewportClient() const
{
	if (!Windows.empty())
		return Windows[0]->GetViewport()->GetLinkedViewportClient();
	return nullptr;
}

IViewportClient* FEditorUI::GetViewportClientAt(int32 Index) const
{
	if (Index < 0 || Index >= (int32)Windows.size()) return nullptr;
	FViewport* VP = Windows[Index]->GetViewport();
	if (!VP) return nullptr;
	return VP->GetLinkedViewportClient();
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
				VP->HandleFileDoubleClick(FilePath);
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
				for (SWindow* Win : Windows)
				{
					FViewport* VP = Win->GetViewport();
					if (VP->IsHovered())
					{
						if (IViewportClient* Client = VP->GetLinkedViewportClient())
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
	if (bViewportActive) return;

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
					if (!Component->IsA(UPrimitiveComponent::StaticClass()))
					{
						continue;
					}

					if (Component->IsA(UStaticMeshComponent::StaticClass()))
					{
						UStaticMeshComponent* StaticMeshComponent = static_cast<UStaticMeshComponent*>(Component);
						FMeshData* MeshData = StaticMeshComponent->GetMeshData();

						if (MeshData)
						{
							Renderer->RenderOutline(
								MeshData,
								StaticMeshComponent->GetWorldTransform()
							);
						}
					}
					else
					{
						UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
						if (PrimitiveComponent->GetPrimitive())
						{
							Renderer->RenderOutline(
								PrimitiveComponent->GetPrimitive()->GetMeshData(),	PrimitiveComponent->GetWorldTransform()
							);
						}

					}
				}
			}

			const float AxisLength = 10000.0f;
			const FVector Origin = { 0.0f, 0.0f, 0.0f };
				
			
	});

	LoadEditorSettings();
}

// ── DetachFromRenderer ──────────────────────────────────────────────────────

void FEditorUI::DetachFromRenderer(FRenderer* InRenderer)
{
	bViewportActive = false;
	CurrentRenderer = nullptr;

	for (SWindow* Win : Windows)
	{
		FViewport* VP = Win->GetViewport();
		VP->ReleaseLevelView();

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
	MainWindow->AddMessageFilter(std::bind(&FEditorUI::HandleInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

#pragma region MultiViewport Layout
	//RootWindow 설정

	
	RootWindow = new SSplitterH();	//가로 splitter bar

	SSplitterV* sideUP = new SSplitterV();	SSplitterV* sideBottom = new SSplitterV();

	SWindow* sideLeftUP, * sideLeftBottom, * sideRightUp, * sideRightBottom;

	sideLeftUP = new SWindow; sideLeftBottom = new SWindow; sideRightUp = new SWindow; sideRightBottom = new SWindow;

	Windows.push_back(sideLeftUP); Windows.push_back(sideRightUp); Windows.push_back(sideLeftBottom); Windows.push_back(sideRightBottom);

	// 각 리프 노드에 FViewport 생성
	sideLeftUP->SetViewport(std::make_unique<FViewport>());
	sideRightUp->SetViewport(std::make_unique<FViewport>());
	sideLeftBottom->SetViewport(std::make_unique<FViewport>());
	sideRightBottom->SetViewport(std::make_unique<FViewport>());

	sideUP->SetSideLT(sideLeftUP); sideUP->SetSideRB(sideRightUp);
	sideBottom->SetSideLT(sideLeftBottom); sideBottom->SetSideRB(sideRightBottom);

	RootWindow->SetSideLT(sideUP); RootWindow->SetSideRB(sideBottom);

	FRect rect = { 0,0,1000,1000 };
	RootWindow->Initialize(rect);

	
	//RootWindow = new SSplitterV();	

	//SSplitterH* sideRight1 = new SSplitterH();	SSplitterH* sideRight2 = new SSplitterH();

	//SWindow* side1, * side2, * side3, * side4;	//왼쪽, 오른쪽 위, 오른쪽 중간, 오른쪽 하단

	//side1 = new SWindow; side2 = new SWindow; side3 = new SWindow; side4 = new SWindow;

	//Windows.push_back(side1); Windows.push_back(side2); Windows.push_back(side3); Windows.push_back(side4);

	//// 각 리프 노드에 FViewport 생성
	//side1->SetViewport(std::make_unique<FViewport>());
	//side2->SetViewport(std::make_unique<FViewport>());
	//side3->SetViewport(std::make_unique<FViewport>());
	//side4->SetViewport(std::make_unique<FViewport>());

	//sideRight1->SetSideLT(side2); sideRight1->SetSideRB(sideRight2);
	//sideRight2->SetSideLT(side3); sideRight2->SetSideRB(side4);

	//RootWindow->SetSideLT(side1); RootWindow->SetSideRB(sideRight1);

	//FRect rect = { 0,0,1000,1000 };
	//RootWindow->Initialize(rect);
	//sideRight1->SetRatio(0.3f);

#pragma endregion
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

	// ── 4분할 씬 뷰포트 ─────────────────────────────────────────────────
	// 하나의 ImGui 창 안에 4개의 씬을 배치
	// Gizmo 버튼은 Viewports[0](주 뷰포트) 기준
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		const bool bVPOpen = ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar);
		ImGui::PopStyleVar();

		if (bVPOpen)
		{

#pragma region Render MultiViewport

			// ── 메뉴바 (Gizmo 버튼) ─────────────────────────────────
			if (ImGui::BeginMenuBar())
			{
				FEditorViewportClient* EditorVP =
					dynamic_cast<FEditorViewportClient*>(GetPrimaryViewportClient());

				if (EditorVP)
				{
					ImGui::Separator();
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, ImGui::GetStyle().ItemSpacing.y));

					auto GizmoBtn = [&](const char* Label, EGizmoMode Mode)
						{
							const bool bSel = EditorVP->GetGizmoMode() == Mode;
							const float H = ImGui::GetFrameHeight();
							if (bSel)
							{
								ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.45f, 0.85f, 1.f));
								ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.55f, 0.95f, 1.f));
								ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.40f, 0.80f, 1.f));
							}
							if (ImGui::Button(Label, ImVec2(H, H)))
								EditorVP->SetGizmoMode(Mode);
							if (bSel) ImGui::PopStyleColor(3);
						};

					GizmoBtn("T", EGizmoMode::Location);
					GizmoBtn("R", EGizmoMode::Rotation);
					GizmoBtn("S", EGizmoMode::Scale);
					ImGui::PopStyleVar();

					// RenderMode 콤보 — 오른쪽 정렬
					float ComboW = 120.f;
					ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ComboW);
					ImGui::SetNextItemWidth(ComboW);
					{
						ERenderMode Mode = EditorVP->GetRenderMode();
						ImGui::Combo("##RM", (int*)&Mode, "Lighting\0No Lighting\0Wireframe", 3);
						EditorVP->SetRenderMode(Mode);
					}
				}
				ImGui::EndMenuBar();
			}


			// ── 4분할 영역 계산 ──────────────────────────────────────
			const ImVec2 Origin = ImGui::GetCursorScreenPos();
			const ImVec2 Total = ImGui::GetContentRegionAvail();
			const float  HalfW = Total.x * 0.5f;
			const float  HalfH = Total.y * 0.5f;
			const HWND   Hwnd = MainWindow ? MainWindow->GetHwnd() : nullptr;

			//SWindow 크기 계산
			// Render() 의 Viewport Begin 블록 안에서
			ImVec2 windowPos = ImGui::GetWindowPos();
			CachedViewportScreenPos = { Origin.x, Origin.y }; // ImVec2 멤버변수
			const FRect NewRect = { Origin.x,Origin.y,Total.y, Total.x };
			RootWindow->UpdateNewSize(NewRect);

			const ImVec2 MousePos = ImGui::GetMousePos();

			for (int i = 0; i < static_cast<int>(Windows.size()) && i < 4; i++)
			{
				/*const Region& R = Regions[i];
				FViewport& VP = Viewports[i];*/
				SWindow* Win = Windows[i];
				FViewport* VP = Win->GetViewport();         // ← SWindow 소유
				const FRect& R = Win->GetWindowSize(); // ← 같은 객체에서

				// hover/focus 계산
				const ImVec2 RMin(R.TopLeftX, R.TopLeftY);
				const ImVec2 RMax(R.TopLeftX + R.Width, R.TopLeftY + R.Height);
				const ImVec2 RSize(R.Width, R.Height);

				// hover / focus
				const bool bHov = ImGui::IsMouseHoveringRect(RMin, RMax, false);
				const bool bFoc = bHov && ImGui::IsMouseDown(ImGuiMouseButton_Left);
				VP->SetHovered(bHov);
				VP->SetFocused(bFoc);

				// 렌더
				VP->PrepareAndUpdate(CurrentRenderer, Hwnd, R.TopLeftX, R.TopLeftY, R.Width, R.Height);

				// DrawList
				ImDrawList* DL = ImGui::GetForegroundDrawList();
				DL->PushClipRect(RMin, RMax, true);

				// =======================
				// Scene
				// =======================
				if (ID3D11ShaderResourceView* SRV = VP->GetSRV())
				{
					DL->AddImage(
						reinterpret_cast<ImTextureID>(SRV),
						RMin,
						RMax
					);
				}

				// =======================
				// Overlay
				// =======================
				FShowFlags& ShowFlags = VP->GetLinkedViewportClient()->GetShowFlags();

				auto ToMB = [](size_t Bytes)
					{
						return Bytes / (1024.0f * 1024.0f);
					};

				// -----------------------
				// FPS (좌상단)
				// -----------------------
				if (ShowFlags.HasFlag(EEngineShowFlags::SF_StatFPS))
				{
					char buf1[64];
					char buf2[64];

					sprintf_s(buf1, "%.2f FPS", Core->GetTimer().GetFPS());
					sprintf_s(buf2, "%.2f ms", Core->GetTimer().GetFrameTimeMs());

					ImVec2 pos = ImVec2(RMin.x + 8.0f, RMin.y + 8.0f);

					DL->AddText(pos, IM_COL32(255, 255, 255, 255), buf1);
					DL->AddText(
						ImVec2(pos.x, pos.y + 16.0f),
						IM_COL32(200, 200, 200, 255),
						buf2
					);
				}

				// -----------------------
				// Memory (정중앙)
				// -----------------------
				if (ShowFlags.HasFlag(EEngineShowFlags::SF_StatMemory))
				{
					const FMallocStats MallocStats = GetGMalloc()->MallocStats;

					char buf[256];
					sprintf_s(buf,
						"Memory\n"
						"Current: %.2f MB\n"
						"Total  : %.2f MB",
						ToMB(MallocStats.CurrentAllocationBytes),
						ToMB(MallocStats.TotalAllocationBytes)
					);

					ImVec2 textSize = ImGui::CalcTextSize(buf);

					ImVec2 center = ImVec2(
						RMin.x + RSize.x * 0.5f - textSize.x * 0.5f,
						RMin.y + RSize.y * 0.5f - textSize.y * 0.5f
					);

					// 배경 박스 (옵션)
					DL->AddRectFilled(
						ImVec2(center.x - 4.0f, center.y - 4.0f),
						ImVec2(center.x + textSize.x + 4.0f, center.y + textSize.y + 4.0f),
						IM_COL32(0, 0, 0, 120),
						4.0f
					);

					DL->AddText(center, IM_COL32(255, 255, 255, 255), buf);
				}

				DL->PopClipRect();
#pragma endregion

				////viewport당 imgui 출력
				//FEditorViewportClient* EditorVP =
				//	dynamic_cast<FEditorViewportClient*>(VP->GetLinkedViewportClient());

				//if (EditorVP) {
				//	float ComboW = 120.f;
				//	ImGui::SetCursorPosX(Windows[i]->GetWindowSize().TopLeftX + ComboW);
				//	ImGui::SetCursorPosY(Windows[i]->GetWindowSize().TopLeftY );

				//	ImGui::SetNextItemWidth(ComboW);
				//	{
				//		ERenderMode Mode = EditorVP->GetRenderMode();
				//		ImGui::Combo("##RM", (int*)&Mode, "Lighting\0No Lighting\0Wireframe", 3);
				//		EditorVP->SetRenderMode(Mode);
				//	}
				//}



			
			}
#pragma region Viewport Splitter

			ImGuiWindowFlags flags =
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoInputs |    // 입력은 기존 시스템이 처리
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoBringToFrontOnFocus;
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			RenderSplitterBars(RootWindow, drawList);
#pragma endregion
		}
		else
		{
			// 창이 닫혔을 때 모든 VP 비활성
			for (SWindow* Win : Windows)
			{
				FViewport* VP = Win->GetViewport();
				VP->SetHovered(false);
				VP->SetFocused(false);
			}
		}
		ImGui::End();
	}


#if IS_OBJ_VIEWER

#else
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
					if (IViewportClient* VP = GetPrimaryViewportClient())
					{
						UCameraComponent* Cam = VP->GetActiveCamera();
						if (Cam) { Cam->SetPosition({ -5.f, 0.f, 2.f }); Cam->SetRotation(0.f, 0.f); }
					}
					Core->GetLevel()->ClearActors();
					UE_LOG("New Level created");
				}
			}

			if (ImGui::MenuItem("Open Level"))
			{
				if (Core && Core->GetActiveLevel())
				{
					FString Path = GetJsonFilePath(EFileDialogType::Open);

					if (!Path.empty())
					{
						Core->SetSelectedActor(nullptr);
						Core->GetLevel()->ClearActors();

						UCameraComponent* Cam = nullptr;
						if (IViewportClient* VP = GetPrimaryViewportClient())
							Cam = VP->GetActiveCamera();

						bool bLoaded = FSceneSerializer::Load(
							Core->GetLevel(), Path, Core->GetRenderer()->GetDevice(), Cam);

						if (bLoaded) UE_LOG("Level loaded: %s", Path.c_str());
						else MessageBoxW(nullptr, L"Level 정보가 잘못되었습니다.", L"Error", MB_OK | MB_ICONWARNING);
					}
				}
			}

			if (ImGui::MenuItem("Save Level As..."))
			{
				if (Core && Core->GetActiveLevel())
				{
					FString Path = GetJsonFilePath(EFileDialogType::Save);

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
		ImVec2 WPos = ImGui::GetWindowPos(), WSize = ImGui::GetWindowSize();
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

#endif

}

// ── GetViewportMousePosition ────────────────────────────────────────────────

bool FEditorUI::GetViewportMousePosition(int32 WindowMouseX, int32 WindowMouseY,
	int32& OutViewportX, int32& OutViewportY,
	int32& OutWidth, int32& OutHeight) const
{
	for (SWindow* Win : Windows)
	{
		FViewport* VP = Win->GetViewport();
		if (VP->IsHovered() || VP->IsFocused())
		{
			return VP->GetMousePositionInViewport(
				WindowMouseX, WindowMouseY,
				OutViewportX, OutViewportY, OutWidth, OutHeight);
		}
	}
	if (!Windows.empty())
	{
		return Windows[0]->GetViewport()->GetMousePositionInViewport(
			WindowMouseX, WindowMouseY,
			OutViewportX, OutViewportY, OutWidth, OutHeight);
	}
	return false;
}

bool FEditorUI::IsViewportInteractive() const
{
	for (SWindow* Win : Windows)
	{
		FViewport* VP = Win->GetViewport();
		if (VP->IsVisible() && (VP->IsHovered() || VP->IsFocused()))
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

//Swindow 입력 처리

bool FEditorUI::HandleInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	//SWindow 입력 처리 (ImGui -> Swindow -> InputManager)
	//TODO : CControlPanel / CProperty 등 여러 UI에 뿌리기

	POINTS mousePosition = MAKEPOINTS(LParam);
	FPoint CurrentPos = { (float)mousePosition.x, (float)mousePosition.y };
	POINT Pt = { static_cast<LONG>(mousePosition.x), static_cast<LONG>(mousePosition.y) };
	::ClientToScreen(Hwnd, &Pt);	//윈도우 창 마우스 위치 -> imgui 좌표계로 변환

	// 모든 마우스 입력 => 포커스 체크
	const bool bIsMouseMsg =
		Msg == WM_LBUTTONDOWN ||
		Msg == WM_RBUTTONDOWN ||
		Msg == WM_MOUSEWHEEL ||
		Msg == WM_MOUSEHWHEEL;

	if (bIsMouseMsg)
	{
		const FRect& R = RootWindow->GetWindowSize();
		/** 화면에 해당하는 RootWindow 위에서만 Focus 체크 */
		if (Pt.x >= R.TopLeftX && Pt.x <= R.TopLeftX + R.Width && Pt.y >= R.TopLeftY && Pt.y <= R.TopLeftY + R.Height)
		{
			Core->SetLastFocusedVP(GetFocusedViewportClient());
			/** VP Focus 시 기존 Console Input Text 등 비활성화 */
			ImGui::ClearActiveID();
			GetConsole().ClearBuffer();
		}
	}

	if (Msg == WM_LBUTTONDOWN)
	{

		RootWindow->AddIfMouseHoverOnBar({ (float)Pt.x, (float)Pt.y }, DraggedSplitters);

		bIsCliked = true;
		PreviousMousePoint = { (float)CurrentPos.PointX, (float)CurrentPos.PointY };
		DeltaMouse.X = 0;
		DeltaMouse.Y = 0;
	}
	else if (Msg == WM_LBUTTONUP)
	{
		bIsCliked = false;
		DraggedSplitters.clear();
	}
	else if (Msg == WM_MOUSEMOVE)
	{
		if (bIsCliked)
		{
			DeltaMouse.X = CurrentPos.PointX - PreviousMousePoint.X;
			DeltaMouse.Y = CurrentPos.PointY - PreviousMousePoint.Y;
			PreviousMousePoint = { (float)CurrentPos.PointX, (float)CurrentPos.PointY };

			// 드래그 중 재판정 없이 클릭 시점 상태 유지
			for (auto& DraggedSplitter : DraggedSplitters) 
			{
				{
					//일반 레이아웃 
					if (DraggedSplitter == RootWindow)
						DraggedSplitter->UpdateBarPosition({ (float)DeltaMouse.X,(float)DeltaMouse.Y });

					else {
						//특정 레이아웃일때
						if (SSplitter* lt = dynamic_cast<SSplitter*>(RootWindow->GetSideLT())) {
							lt->UpdateBarPosition({ (float)DeltaMouse.X,(float)DeltaMouse.Y });
						}
						if (SSplitter* rb = dynamic_cast<SSplitter*>(RootWindow->GetSideRB())) {
							rb->UpdateBarPosition({ (float)DeltaMouse.X,(float)DeltaMouse.Y });
						}
					}
					UE_LOG("Dragging bar: %f, %f", DeltaMouse.X, DeltaMouse.Y);
				}

				//DraggedSplitter->UpdateBarPosition({ (float)DeltaMouse.X,(float)DeltaMouse.Y });
			}
		}

		if(RootWindow->IsAnyBarHovered({ (float)Pt.x, (float)Pt.y }))
			SetCursor(LoadCursor(NULL, IDC_HAND)); // 손가락 모양으로 변경
	}

	return false;
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

FViewport* FEditorUI::GetViewportAt(int32 Index)
{
	if (Index < 0 || Index >= (int32)Windows.size()) return nullptr;
	return Windows[Index]->GetViewport();
}

void FEditorUI::RenderSplitterBars(SSplitter* splitter, ImDrawList* drawList)
{
	if (!splitter) return;

	// 자식 스플리터도 재귀로 그리기
	if (auto* childLT = dynamic_cast<SSplitter*>(splitter->GetSideLT()))
		RenderSplitterBars(childLT, drawList);
	if (auto* childRB = dynamic_cast<SSplitter*>(splitter->GetSideRB()))
		RenderSplitterBars(childRB, drawList);


	FRect bar = splitter->GetBarRect(); // 바 위치/크기 가져오기


	// ImGui 좌표로 변환
	ImVec2 barMin = ImVec2(bar.TopLeftX, bar.TopLeftY);
	ImVec2 barMax = ImVec2(bar.TopLeftX + bar.Width, bar.TopLeftY + bar.Height);

	// 마우스 호버 체크
	ImVec2 mousePos = ImGui::GetMousePos();
	FPoint mousePoint = { mousePos.x, mousePos.y };
	bool bHovered = splitter->isMouseHoverOnBar(mousePoint);

	// 호버면 흰색, 아니면 회색
	ImU32 color = bHovered
		? IM_COL32(255, 255, 255, 255)  // 흰색
		: IM_COL32(100, 100, 100, 255); // 회색

	drawList->AddRectFilled(barMin, barMax, color);

	
}
