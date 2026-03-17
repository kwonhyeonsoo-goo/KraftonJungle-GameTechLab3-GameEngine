#include "EditorViewportClient.h"

#include "EditorUI.h"
#include "Core/Core.h"
#include "Platform/Windows/Window.h"
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
		if (IO.WantCaptureKeyboard || IO.WantCaptureMouse)
		{
			return;
		}
	}

	IViewportClient::Tick(Core, DeltaTime);
}

void CEditorViewportClient::HandleMessage(CCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (!Core || Msg != WM_LBUTTONDOWN)
	{
		return;
	}

	if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse)
	{
		return;
	}

	UScene* Scene = ResolveScene(Core);
	if (!Scene)
	{
		return;
	}

	RECT Rect = {};
	GetClientRect(Hwnd, &Rect);
	const int32 Width = Rect.right - Rect.left;
	const int32 Height = Rect.bottom - Rect.top;
	if (Width <= 0 || Height <= 0)
	{
		return;
	}

	const int32 MouseX = LOWORD(LParam);
	const int32 MouseY = HIWORD(LParam);
	AActor* PickedActor = Picker.PickActor(Scene, MouseX, MouseY, Width, Height);
	if (PickedActor)
	{
		Core->SetSelectedActor(PickedActor);
	}
}
