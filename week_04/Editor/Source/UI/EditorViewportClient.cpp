#include "EditorViewportClient.h"

#include "EditorUI.h"
#include "Actor/Actor.h"
#include "Core/Core.h"
#include "Input/InputManager.h"
#include "Debug/EngineLog.h"
#include "Platform/Windows/Window.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"

#include "imgui.h"

CEditorViewportClient::CEditorViewportClient(CEditorUI& InEditorUI, CWindow* InMainWindow)
	: EditorUI(InEditorUI)
	, MainWindow(InMainWindow)
{
}

void CEditorViewportClient::Attach(CCore* Core, CRenderer* Renderer)
{
	if (!Core || !Renderer || !MainWindow)
	{
		return;
	}

	EditorUI.Initialize(Core);
	EditorUI.SetupWindow(MainWindow);
	EditorUI.AttachToRenderer(Renderer);
}

void CEditorViewportClient::Detach(CCore* Core, CRenderer* Renderer)
{
	Gizmo.EndDrag();
	EditorUI.DetachFromRenderer(Renderer);
}

void CEditorViewportClient::Tick(CCore* Core, float DeltaTime)
{
	if (!Core)
	{
		return;
	}

	if (ImGui::GetCurrentContext())
	{
		const ImGuiIO& IO = ImGui::GetIO();
		if ((IO.WantCaptureKeyboard || IO.WantCaptureMouse) && !EditorUI.IsViewportInteractive())
		{
			return;
		}
	}

	if (!EditorUI.IsViewportInteractive())
	{
		return;
	}

	IViewportClient::Tick(Core, DeltaTime);
}

void CEditorViewportClient::HandleMessage(CCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (!Core || !EditorUI.IsViewportInteractive())
	{
		return;
	}

	if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse && !EditorUI.IsViewportInteractive())
	{
		return;
	}

	UScene* Scene = ResolveScene(Core);
	AActor* SelectedActor = Core->GetSelectedActor();
	if (!Scene)
	{
		return;
	}

	int32 MouseX = 0;
	int32 MouseY = 0;
	int32 Width = 0;
	int32 Height = 0;
	const bool bHasViewportMouse = EditorUI.GetViewportMousePosition(
		static_cast<int32>(static_cast<short>(LOWORD(LParam))),
		static_cast<int32>(static_cast<short>(HIWORD(LParam))),
		MouseX,
		MouseY,
		Width,
		Height);

	const bool bRightMouseDown = Core->GetInputManager() &&
		Core->GetInputManager()->IsMouseButtonDown(CInputManager::MOUSE_RIGHT);

	switch (Msg)
	{
	case WM_KEYDOWN:
		if (bRightMouseDown)
		{
			return;
		}

		switch (WParam)
		{
		case 'W':
			Gizmo.SetMode(EGizmoMode::Location);
			return;

		case 'E':
			Gizmo.SetMode(EGizmoMode::Rotation);
			return;

		case 'R':
			Gizmo.SetMode(EGizmoMode::Scale);
			return;

		case 'L':
			Gizmo.ToggleCoordinateSpace();
			UE_LOG("Gizmo Space: %s", Gizmo.GetCoordinateSpace() == EGizmoCoordinateSpace::Local ? "Local" : "World");
			return;

		default:
			return;
		}

	case WM_LBUTTONDOWN:
		if (!bHasViewportMouse)
		{
			return;
		}

		if (SelectedActor && Gizmo.BeginDrag(SelectedActor, Scene, Picker, MouseX, MouseY, Width, Height))
		{
			return;
		}

		if (AActor* PickedActor = Picker.PickActor(Scene, MouseX, MouseY, Width, Height))
		{
			Core->SetSelectedActor(PickedActor);
			EditorUI.SyncSelectedActorProperty();
		}
		return;

	case WM_MOUSEMOVE:
		if (!bHasViewportMouse)
		{
			Gizmo.ClearHover();
			return;
		}

		if (!Gizmo.IsDragging())
		{
			Gizmo.UpdateHover(SelectedActor, Scene, Picker, MouseX, MouseY, Width, Height);
			return;
		}

		if (Gizmo.UpdateDrag(SelectedActor, Scene, Picker, MouseX, MouseY, Width, Height))
		{
			EditorUI.SyncSelectedActorProperty();
		}
		return;

	case WM_LBUTTONUP:
		if (Gizmo.IsDragging())
		{
			Gizmo.EndDrag();
			if (bHasViewportMouse)
			{
				Gizmo.UpdateHover(SelectedActor, Scene, Picker, MouseX, MouseY, Width, Height);
			}
			else
			{
				Gizmo.ClearHover();
			}
			EditorUI.SyncSelectedActorProperty();
		}
		return;

	default:
		return;
	}
}

void CEditorViewportClient::BuildRenderCommands(CCore* Core, UScene* Scene, const FFrustum& Frustum, FRenderCommandQueue& OutQueue) const
{
	IViewportClient::BuildRenderCommands(Core, Scene, Frustum, OutQueue);

	if (!Core || !Scene || !Scene->GetCamera())
	{
		return;
	}

	Gizmo.BuildRenderCommands(Core->GetSelectedActor(), Scene->GetCamera(), OutQueue);
}
