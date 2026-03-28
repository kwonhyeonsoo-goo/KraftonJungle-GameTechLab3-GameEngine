#pragma once

#include "Core/FEngine.h"
#include "UI/EditorUI.h"
#include "Controller/EditorViewportController.h"

class AEditorCameraPawn;
class FEditorViewportClient;
class FPreviewViewportClient;

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
	void CreateViewportClients() override;

	FEditorViewportController* GetViewportController();

private:
	FEditorUI EditorUI;
	AEditorCameraPawn* EditorPawn = nullptr;
	FEditorViewportController ViewportController;

	// 배열 내 포인터 — 소유권은 ViewportClientArray
	FEditorViewportClient* EditorViewportClient = nullptr;
	FPreviewViewportClient* PreviewViewportClient = nullptr;
};