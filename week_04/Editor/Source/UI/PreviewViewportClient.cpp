#include "PreviewViewportClient.h"

#include "EditorUI.h"
#include "Core/Core.h"
#include "Platform/Windows/Window.h"
#include "Renderer/Renderer.h"

#include "imgui.h"

FPreviewViewportClient::FPreviewViewportClient(FEditorUI& InEditorUI, FWindow* InMainWindow, FString InPreviewContextName)
	: EditorUI(InEditorUI)
	, MainWindow(InMainWindow)
	, PreviewContextName(std::move(InPreviewContextName))
{
}

void FPreviewViewportClient::Attach(FCore* Core, FRenderer* Renderer)
{
	if (!Core || !Renderer || !MainWindow)
	{
		return;
	}

	EditorUI.Initialize(Core);
	EditorUI.SetupWindow(MainWindow);
	EditorUI.AttachToRenderer(Renderer);
}

void FPreviewViewportClient::Detach(FCore* Core, FRenderer* Renderer)
{
	EditorUI.DetachFromRenderer(Renderer);
}

void FPreviewViewportClient::Tick(FCore* Core, float DeltaTime)
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

ULevel* FPreviewViewportClient::ResolveLevel(FCore* Core) const
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
