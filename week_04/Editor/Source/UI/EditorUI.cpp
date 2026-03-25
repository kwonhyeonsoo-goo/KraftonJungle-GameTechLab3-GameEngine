#include "EditorUI.h"

#include "Core/Core.h"
#include "Object/Object.h"
#include "Scene/Scene.h"
#include "Actor/Actor.h"
#include "Component/PrimitiveComponent.h"
#include "Component/SceneComponent.h"
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

#include "Debug/EngineLog.h"
#include "Component/CameraComponent.h"
#include "Camera/Camera.h"
#include "Serializer/SceneSerializer.h"
#include "Actor/SkySphereActor.h" 
#include "Actor/ObjActor.h"

enum class EFileDialogType
{
	Open,
	Save
};

std::string GetFilePathUsingDialog(EFileDialogType Type)
{
	char FileName[MAX_PATH] = "";
	FString ContentDir = FPaths::ContentDir().string();

	OPENFILENAMEA Ofn = {};
	Ofn.lStructSize = sizeof(OPENFILENAMEA);
	Ofn.lpstrFilter = "Scene Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
	Ofn.lpstrFile = FileName;
	Ofn.nMaxFile = MAX_PATH;
	Ofn.lpstrDefExt = "json";
	Ofn.lpstrInitialDir = ContentDir.c_str();

	if (Type == EFileDialogType::Save)
	{
		Ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

		if (GetSaveFileNameA(&Ofn))
			return std::string(FileName);
	}
	else // Open
	{
		Ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		if (GetOpenFileNameA(&Ofn))
			return std::string(FileName);
	}

	return "";
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

void CEditorUI::Initialize(CCore* InCore)
{
	Core = InCore;

	Property.OnChanged = [this](const FVector& Loc, const FVector& Rot, const FVector& Scl)
		{
			if (!Core)
			{
				return;
			}

			AActor* Selected = Core->GetSelectedActor();
			if (!Selected)
			{
				return;
			}

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
			if (Core)
			{

				Core->GetViewportClient()->HandleFileDoubleClick(FilePath);
			}
		};

	ContentBrowser.OnFileDragEnd = [this](const FString& DraggingFilePath, const FString& ReleaseDirectory)
		{
			if (ContentBrowser.IsHovered())
			{
				if (ContentBrowser.IsMouseOnDirectory())
				{
					std::filesystem::path Src = DraggingFilePath;
					std::filesystem::path DstDir = ReleaseDirectory;

					std::filesystem::path Dst = DstDir / Src.filename();

					std::error_code ec;

					if (std::filesystem::exists(Dst))
					{
						int Result = MessageBoxW(
							nullptr,
							L"이미 같은 이름의 파일이 존재합니다.\n덮어쓰시겠습니까?",
							L"Overwrite",
							MB_YESNO | MB_ICONWARNING
						);

						if (Result != IDYES)
						{
							return; // 취소
						}

						// 덮어쓰기 위해 기존 파일 삭제
						std::filesystem::remove(Dst, ec);
						if (ec)
						{
							MessageBoxW(nullptr, L"Delete Failed", L"Error", MB_OK | MB_ICONERROR);
							return;
						}
					}

					std::filesystem::rename(Src, Dst, ec);

					if (ec)
					{
						UE_LOG("Move Failed: %s", ec.message().c_str());
					}
					else
					{
						UE_LOG("Moved: %s -> %s", Src.string().c_str(), Dst.string().c_str());
					}
				}
			}
			else if (Viewport.IsHovered())
			{
				UE_LOG("Drop On Viewport");			
				if (Core)
				{
					Core->GetViewportClient()->HandleFileDropOnViewport(DraggingFilePath);
				}
			}
		};
}

void CEditorUI::AttachToRenderer(CRenderer* InRenderer)
{
	if (!Core || !InRenderer)
	{
		return;
	}

	bViewportClientActive = true;
	CurrentRenderer = InRenderer;

	const HWND Hwnd = InRenderer->GetHwnd();
	ID3D11Device* Device = InRenderer->GetDevice();
	ID3D11DeviceContext* DeviceContext = InRenderer->GetDeviceContext();

	ContentBrowser.SetFolderIcon(CurrentRenderer->GetFolderIconSRV());
	ContentBrowser.SetFileIcon(CurrentRenderer->GetFileIconSRV());

	std::filesystem::path FontPath = FPaths::ProjectRoot() / "Content" / "Fonts" / "NotoSansKR-Bold.ttf";
	std::string FontPathString = FontPath.string();
	std::wstring FontPathWString = FontPath.wstring();
	InRenderer->SetGUICallbacks(
		[Hwnd, Device, DeviceContext, FontPathString, FontPathWString]()
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

			ImFont* Font = nullptr;

			if (std::filesystem::exists(FontPathString))
			{
				Font = IO.Fonts->AddFontFromFileTTF(
					FontPathString.c_str(),
					16.0f,
					&FontConfig,
					IO.Fonts->GetGlyphRangesKorean()
				);
			}

			if (!Font)
			{
				MessageBoxW(nullptr, FontPathWString.c_str(), L"Failed to load font", MB_OK);
				IO.Fonts->AddFontDefault();
			}

			ImGui::StyleColorsDark();

			ImGuiStyle& Style = ImGui::GetStyle();
			Style.WindowPadding = ImVec2(0, 0);
			Style.DisplayWindowPadding = ImVec2(0, 0);
			Style.DisplaySafeAreaPadding = ImVec2(0, 0);

			Style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			Style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);


			if (IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				Style.WindowRounding = 0.0f;
				Style.Colors[ImGuiCol_WindowBg].w = 1.0f;
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
			ImGuiIO& IO = ImGui::GetIO();
			if (IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
		}
	);

	InRenderer->SetGUIUpdateCallback([this]() { Render(); });

	InRenderer->SetPostRenderCallback([this](CRenderer* Renderer)
		{
			if (!Core)
			{
				return;
			}
	
			AActor* Selected = Core->GetSelectedActor();
			if (Selected && !Selected->IsPendingDestroy() && Selected->IsVisible()
				&& !Selected->IsA<ASkySphereActor>()
				&& Core->GetViewportClient()->GetShowFlags().HasFlag(EEngineShowFlags::SF_Primitives))
			{
				for (UActorComponent* Component : Selected->GetComponents())
				{
					if (!Component->IsA(UPrimitiveComponent::StaticClass()))
					{
						continue;
					}

					UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
					if (PrimitiveComponent->GetPrimitive())
					{
						Renderer->RenderOutline(
							PrimitiveComponent->GetPrimitive()->GetMeshData(),
							PrimitiveComponent->GetWorldTransform()
						);
					}
				}
			}

			const float AxisLength = 10000.0f;
			const FVector Origin = { 0.0f, 0.0f, 0.0f };
		});
}

void CEditorUI::DetachFromRenderer(CRenderer* InRenderer)
{
	bViewportClientActive = false;
	CurrentRenderer = nullptr;
	Viewport.ReleaseSceneView();

	if (InRenderer)
	{
		InRenderer->ClearSceneRenderTarget();
		InRenderer->ClearViewportCallbacks();
	}
}

void CEditorUI::SetupWindow(CWindow* InWindow)
{
	MainWindow = InWindow;
	if (bWindowSetup || MainWindow == nullptr)
	{
		return;
	}

	bWindowSetup = true;

	MainWindow->AddMessageFilter([this](HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam) -> bool
		{
			if (!bViewportClientActive)
			{
				return false;
			}

			const bool bIsImeMessage =
				Msg == WM_IME_STARTCOMPOSITION ||
				Msg == WM_IME_COMPOSITION ||
				Msg == WM_IME_ENDCOMPOSITION ||
				Msg == WM_IME_NOTIFY ||
				Msg == WM_IME_SETCONTEXT ||
				Msg == WM_IME_CHAR;

			const bool bIsCharMessage =
				Msg == WM_CHAR ||
				Msg == WM_SYSCHAR ||
				Msg == WM_UNICHAR;

			if (bIsImeMessage || bIsCharMessage)
			{
				if (ImGui::GetCurrentContext())
				{
					const ImGuiIO& IO = ImGui::GetIO();
					if (!IO.WantTextInput)
					{
						return true;
					}
				}
				else
				{
					return true;
				}
			}

			const bool bHandledByImGui = ImGui_ImplWin32_WndProcHandler(Hwnd, Msg, WParam, LParam) != 0;

			if (IsViewportInteractive())
			{
				return false;
			}

			return bHandledByImGui;
		});
}

void CEditorUI::BuildDefaultLayout(uint32 DockID)
{
	ImGui::DockBuilderRemoveNode(DockID);
	ImGui::DockBuilderAddNode(DockID, ImGuiDockNodeFlags_DockSpace);

	ImGuiViewport* Viewport = ImGui::GetMainViewport();
	ImGui::DockBuilderSetNodeSize(DockID, Viewport->WorkSize);

	ImGuiID DockBottom = 0;
	ImGuiID DockUpper = 0;
	ImGui::DockBuilderSplitNode(DockID, ImGuiDir_Down, 0.25f, &DockBottom, &DockUpper);

	ImGuiID DockLeft = 0;
	ImGuiID DockCenter = 0;
	ImGui::DockBuilderSplitNode(DockUpper, ImGuiDir_Left, 0.20f, &DockLeft, &DockCenter);

	ImGuiID DockRight = 0;
	ImGui::DockBuilderSplitNode(DockCenter, ImGuiDir_Right, 0.25f, &DockRight, &DockCenter);

	ImGuiID DockRightTop = 0;
	ImGuiID DockRightBottom = 0;
	ImGui::DockBuilderSplitNode(DockRight, ImGuiDir_Up, 0.50f, &DockRightTop, &DockRightBottom);
	ImGui::DockBuilderDockWindow("Viewport", DockCenter);
	ImGui::DockBuilderDockWindow("Viewport", DockCenter);
	ImGui::DockBuilderDockWindow("Stats", DockLeft);
	ImGui::DockBuilderDockWindow("Properties", DockRightTop);
	ImGui::DockBuilderDockWindow("Control Panel", DockRightBottom);
	ImGui::DockBuilderDockWindow("Console", DockBottom);

	ImGui::DockBuilderFinish(DockID);
}

void CEditorUI::Render()
{
	if (!bViewportClientActive)
	{
		return;
	}

	ImGuiViewport* MainViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(MainViewport->WorkPos);
	ImGui::SetNextWindowSize(MainViewport->WorkSize);
	ImGui::SetNextWindowViewport(MainViewport->ID);

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

	Viewport.Render(Core, CurrentRenderer, MainWindow ? MainWindow->GetHwnd() : nullptr);

	if (Core)
	{
		AActor* Selected = Core->GetSelectedActor();
		if (Selected != CachedSelectedActor)
		{
			SyncSelectedActorProperty();
		}

		const FTimer& Timer = Core->GetTimer();
		Stat.SetFPS(Timer.GetDisplayFPS());
		Stat.SetFrameTimeMs(Timer.GetFrameTimeMs());
	}

	Stat.SetObjectCount(UObject::TotalAllocationCounts);
	Stat.SetHeapUsage(UObject::TotalAllocationBytes);

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Scene"))
			{
				if (Core)
				{
					Core->SetSelectedActor(nullptr);

					if (UCameraComponent* Cam = Core->GetActiveWorld()->GetActiveCameraComponent())
					{
						Cam->GetCamera()->SetPosition({ -5.0f, 0.0f, 2.0f });
						Cam->GetCamera()->SetRotation(0.f, 0.f);
					}
					Core->GetScene()->ClearActors();
					UE_LOG("New scene created");
				}
			}

			if (ImGui::MenuItem("Open Scene"))
			{
				if (Core && Core->GetActiveScene())
				{
					FString Path = GetFilePathUsingDialog(EFileDialogType::Open);

					if (!Path.empty())
					{
						Core->SetSelectedActor(nullptr);
						Core->GetScene()->ClearActors();

						bool bLoaded = FSceneSerializer::Load(Core->GetScene(), Path, Core->GetRenderer()->GetDevice());
						if (bLoaded)
						{
							UE_LOG("Scene loaded: %s", Path.c_str());
						}
						else
						{
							MessageBoxW(
								nullptr,
								L"Scene 정보가 잘못되었습니다.",
								L"Error",
								MB_OK | MB_ICONWARNING
							);
						}
					}
				}
			}

			if (ImGui::MenuItem("Save Scene As..."))
			{
				if (Core && Core->GetActiveScene())
				{
					FString Path = GetFilePathUsingDialog(EFileDialogType::Save);

					if (!Path.empty())
					{
						FSceneSerializer::Save(Core->GetScene(),Path);
					}
				}
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	ControlPanel.Render(Core);
	Property.Render(Core);
	Console.Render();
	Stat.Render();
	Outliner.Render(Core);
	ContentBrowser.Render();
}

bool CEditorUI::GetViewportMousePosition(int32 WindowMouseX, int32 WindowMouseY, int32& OutViewportX, int32& OutViewportY, int32& OutWidth, int32& OutHeight) const
{
	return Viewport.GetMousePositionInViewport(WindowMouseX, WindowMouseY, OutViewportX, OutViewportY, OutWidth, OutHeight);
}

void CEditorUI::SyncSelectedActorProperty()
{
	if (!Core)
	{
		return;
	}

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
				Selected->GetName().c_str()
			);
		}
	}
	else
	{
		Property.SetTarget({ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, "None");
	}

	CachedSelectedActor = Selected;
}

bool CEditorUI::IsViewportInteractive() const
{
	return Viewport.IsVisible() && (Viewport.IsHovered() || Viewport.IsFocused());
}
