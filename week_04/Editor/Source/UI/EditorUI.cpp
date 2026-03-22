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

	InRenderer->SetGUICallbacks(
		[Hwnd, Device, DeviceContext]()
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& IO = ImGui::GetIO();
			IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
			IO.IniFilename = "imgui_editor.ini";

			ImGui::StyleColorsDark();

			ImGuiStyle& Style = ImGui::GetStyle();
			Style.WindowPadding = ImVec2(0, 0);
			Style.DisplayWindowPadding = ImVec2(0, 0);
			Style.DisplaySafeAreaPadding = ImVec2(0, 0);
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
			if (Selected && !Selected->IsPendingDestroy() && Selected->IsVisible())
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
			Renderer->DrawLine(Origin, { AxisLength, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
			Renderer->DrawLine(Origin, { 0.0f, AxisLength, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
			Renderer->DrawLine(Origin, { 0.0f, 0.0f, AxisLength }, { 0.0f, 0.0f, 1.0f, 1.0f });
			Renderer->ExecuteLineCommands();
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

	ControlPanel.Render(Core);
	Property.Render();
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
