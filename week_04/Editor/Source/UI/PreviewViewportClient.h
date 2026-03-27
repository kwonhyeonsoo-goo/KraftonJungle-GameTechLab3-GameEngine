#pragma once

#include "CoreMinimal.h"
#include "Core/ViewportClient.h"

class FEditorUI;
class FWindow;

class FPreviewViewportClient : public IViewportClient
{
public:
	FPreviewViewportClient(FEditorUI& InEditorUI, FWindow* InMainWindow, FString InPreviewContextName);

	void Attach(FCore* Core, FRenderer* Renderer) override;
	void Detach(FCore* Core, FRenderer* Renderer) override;
	void Tick(FCore* Core, float DeltaTime) override;
	ULevel* ResolveLevel(FCore* Core) const override;

private:
	FEditorUI& EditorUI;
	FWindow* MainWindow = nullptr;
	FString PreviewContextName;
};
