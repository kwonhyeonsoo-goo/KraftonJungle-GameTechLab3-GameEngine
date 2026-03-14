#include "EditorUI.h"
#include "Core/Core.h"
#include "Renderer/Renderer.h"
#include "Object/Scene/Scene.h"
#include "Object/Actor/Actor.h"
#include "Platform/Windows/Window.h"

#include "imgui.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

void CEditorUI::Initialize(CCore* InCore)
{
	Core = InCore;
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
			int MouseX = LOWORD(l);
			int MouseY = HIWORD(l);
			RECT Rect;
			GetClientRect(h, &Rect);
			int Width = Rect.right - Rect.left;
			int Height = Rect.bottom - Rect.top;

			AActor* Picked = Picker.PickActor(Core->GetScene(), MouseX, MouseY, Width, Height);
			if (Picked)
			{
				SetSelectedActor(Picked);
			}
		}
		return false;
	});

	// Input forwarding filter
	MainWindow->AddMessageFilter([this](HWND h, UINT m, WPARAM w, LPARAM l) -> bool
	{
		if (Core)
		{
			Core->ProcessInput(h, m, w, l);
		}
		return false;
	});

	// Resize callback
	MainWindow->SetOnResizeCallback([this](int W, int H)
	{
		if (Core && Core->GetRenderer())
		{
			Core->GetRenderer()->OnResize(W, H);
		}
	});

	MainWindow->Show();
}

void CEditorUI::Render()
{
	ControlPanel.Render(Core, SelectedActor);
	Property.Render(SelectedActor);
	Console.Render();
	Stat.Render();
}

void CEditorUI::SetSelectedActor(AActor* InActor)
{
	SelectedActor = InActor;
	if (Core)
	{
		Core->SetSelectedActor(InActor);
	}
}
