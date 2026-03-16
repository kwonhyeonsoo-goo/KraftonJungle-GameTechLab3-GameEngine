#include "EditorUI.h"
#include "Core/Core.h"
#include "Camera/Camera.h"
#include "Renderer/Renderer.h"
#include "Object/Scene/Scene.h"
#include "Object/Actor/Actor.h"
#include "Object/Object.h"
#include "Component/SceneComponent.h"
#include "Component/PrimitiveComponent.h"
#include "Primitive/PrimitiveBase.h"
#include "Platform/Windows/Window.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

void CEditorUI::Initialize(CCore* InCore)
{
	Core = InCore;

	// Setup ImGui callbacks
	CRenderer* Renderer = Core->GetRenderer();
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

	// Property change callback
	Property.OnChanged = [this](const FVector& Loc, const FVector& Rot, const FVector& Scl)
		{
			if (Core)
			{
				AActor* Selected = Core->GetSelectedActor();
				if (Selected)
				{
					if (USceneComponent* Root = Selected->GetRootComponent())
					{
						FTransform T = Root->GetRelativeTransform();
						T.SetLocation(Loc);
						T.SetRotation(FRotator::MakeFromEuler(Rot));
						T.SetScale3D(Scl);
						Root->SetRelativeTransform(T);
					}
				}
			}
		};

	Renderer->SetGUIUpdateCallback([this]() { Render(); });

	// Editor-only post-render: outline, world axis, gizmo etc.
	Core->SetPostRenderCallback([this](CRenderer* R)
		{
			if (!Core) return;

			// 선택된 Actor 아웃라인 렌더링
			AActor* Selected = Core->GetSelectedActor();
			if (Selected && !Selected->IsPendingDestroy())
			{
				for (UActorComponent* Comp : Selected->GetComponents())
				{
					UPrimitiveComponent* PrimComp = dynamic_cast<UPrimitiveComponent*>(Comp);
					if (PrimComp && PrimComp->GetPrimitive())
					{
						R->RenderOutline(
							PrimComp->GetPrimitive()->GetMeshData(),
							PrimComp->GetWorldTransform()
						);
					}
				}
			}

			// 월드 원점 축 렌더링 (X=빨강, Y=초록, Z=파랑)
			float AxisLength = 10000.0f;
			FVector Origin = { 0.0f, 0.0f, 0.0f };
			R->DrawLine(Origin, { AxisLength, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
			R->DrawLine(Origin, { 0.0f, AxisLength, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
			R->DrawLine(Origin, { 0.0f, 0.0f, AxisLength }, { 0.0f, 0.0f, 1.0f, 1.0f });
			R->ExecuteLineCommands();
		});
}

void CEditorUI::SetupWindow(CWindow* InWindow)
{
	MainWindow = InWindow;

	// ImGui message filter (must be first)
	MainWindow->AddMessageFilter([](HWND h, UINT m, WPARAM w, LPARAM l) -> bool
		{
			return ImGui_ImplWin32_WndProcHandler(h, m, w, l) != 0;
		});

	// Picking filter
	MainWindow->AddMessageFilter([this](HWND h, UINT m, WPARAM w, LPARAM l) -> bool
		{
			if (m == WM_LBUTTONDOWN && !ImGui::GetIO().WantCaptureMouse && Core && Core->GetScene())
			{
				int32 MouseX = LOWORD(l);
				int32 MouseY = HIWORD(l);
				RECT Rect;
				GetClientRect(h, &Rect);
				int32 Width = Rect.right - Rect.left;
				int32 Height = Rect.bottom - Rect.top;

				AActor* Picked = Picker.PickActor(Core->GetScene(), MouseX, MouseY, Width, Height);
				if (Picked)
				{
					SetSelectedActor(Picked);
				}
			}
			return false;
		});
}

void CEditorUI::BuildDefaultLayout(uint32 DockID)
{
	ImGui::DockBuilderRemoveNode(DockID);
	ImGui::DockBuilderAddNode(DockID, ImGuiDockNodeFlags_DockSpace);

	ImGuiViewport* Viewport = ImGui::GetMainViewport();
	ImGui::DockBuilderSetNodeSize(DockID, Viewport->WorkSize);

	ImGuiID DockBottom, DockUpper;
	ImGui::DockBuilderSplitNode(DockID, ImGuiDir_Down, 0.25f, &DockBottom, &DockUpper);

	ImGuiID DockLeft, DockCenter;
	ImGui::DockBuilderSplitNode(DockUpper, ImGuiDir_Left, 0.20f, &DockLeft, &DockCenter);

	ImGuiID DockRight;
	ImGui::DockBuilderSplitNode(DockCenter, ImGuiDir_Right, 0.25f, &DockRight, &DockCenter);

	ImGuiID DockRightTop, DockRightBottom;
	ImGui::DockBuilderSplitNode(DockRight, ImGuiDir_Up, 0.50f, &DockRightTop, &DockRightBottom);

	ImGui::DockBuilderDockWindow("Stats", DockLeft);
	ImGui::DockBuilderDockWindow("Properties", DockRightTop);
	ImGui::DockBuilderDockWindow("Control Panel", DockRightBottom);
	ImGui::DockBuilderDockWindow("Console", DockBottom);

	ImGui::DockBuilderFinish(DockID);
}

void CEditorUI::Render()
{
	// DockSpace host
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

	// Track selected actor changes for Property
	if (Core)
	{
		AActor* Selected = Core->GetSelectedActor();
		if (Selected != CachedSelectedActor)
		{
			if (Selected)
			{
				USceneComponent* Root = Selected->GetRootComponent();
				if (Root)
				{
					FTransform T = Root->GetRelativeTransform();
					Property.SetTarget(T.GetLocation(), T.Rotator().Euler(), T.GetScale3D(), Selected->GetName().c_str());
				}
			}
			else
			{
				Property.SetTarget({ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, "None");
			}
			CachedSelectedActor = Selected;
		}
	}

	// Update stats
	Stat.SetObjectCount(UObject::TotalAllocationCounts);
	Stat.SetHeapUsage(UObject::TotalAllocationBytes);

	// Panels
	ControlPanel.Render(Core);
	Property.Render();
	Console.Render();
	Stat.Render();
}

void CEditorUI::SetSelectedActor(AActor* InActor)
{
	if (Core)
	{
		Core->SetSelectedActor(InActor);
	}
}
