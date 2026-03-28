#pragma once

#include "Core/FEngine.h"
#include "UI/EditorUI.h"
#include "UI/PreviewViewportClient.h"
#include "Controller/EditorViewportController.h"

class AEditorCameraPawn;

class FEditorEngine : public FEngine
{
public:
	FEditorEngine() = default;
	~FEditorEngine() override;

	bool Initialize(HINSTANCE hInstance);
	void Shutdown() override;

protected:
	void PreInitialize() override;
	void PostInitialize() override;
	void Tick(float DeltaTime) override;
	ELevelType GetStartupLevelType() const override { return ELevelType::Editor; }
	std::unique_ptr<FViewportClient> CreateViewportClient() override;

	FEditorViewportController* GetViewportController();
private:
	void SyncViewportClient();

	FEditorUI EditorUI;
	std::unique_ptr<FPreviewViewportClient> PreviewViewportClient;
	AEditorCameraPawn* EditorPawn = nullptr;
	FEditorViewportController ViewportController;
};
