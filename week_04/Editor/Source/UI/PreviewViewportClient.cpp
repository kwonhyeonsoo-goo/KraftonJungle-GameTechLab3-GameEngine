#include "PreviewViewportClient.h"

#include "EditorUI.h"
#include "Core/Core.h"
#include "Platform/Windows/Window.h"
#include "Renderer/Renderer.h"

#include "imgui.h"

CPreviewViewportClient::CPreviewViewportClient(CEditorUI& InEditorUI, CWindow* InMainWindow, FString InPreviewContextName)
	: EditorUI(InEditorUI)
	, MainWindow(InMainWindow)
	, PreviewContextName(std::move(InPreviewContextName))
{
}

void CPreviewViewportClient::Attach(CCore* Core, CRenderer* Renderer)
{
	if (!Core || !Renderer || !MainWindow)
	{
		return;
	}

	EditorUI.Initialize(Core);
	EditorUI.SetupWindow(MainWindow);
	EditorUI.AttachToRenderer(Renderer);
}

void CPreviewViewportClient::Detach(CCore* Core, CRenderer* Renderer)
{
	EditorUI.DetachFromRenderer(Renderer);
}

void CPreviewViewportClient::Tick(CCore* Core, float DeltaTime)
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

ULevel* CPreviewViewportClient::ResolveLevel(CCore* Core) const
{
	if (!Core)
	{
		return nullptr;
	}

	if (ULevel* PreviewLevel = Core->GetLevelManager()->GetPreviewLevel(PreviewContextName))
	{
		return PreviewLevel;
	}

	return Core->GetActiveLevel();
}
