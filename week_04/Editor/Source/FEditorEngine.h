#pragma once

#include "Core/FEngine.h"
#include "UI/EditorUI.h"
#include "Controller/EditorViewportController.h"

class AEditorCameraPawn;
class FEditorViewportClient;

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

	//FEditorViewportController* GetViewportController();

private:
	FEditorUI EditorUI;
	TArray<std::unique_ptr<FEditorViewportController>> ViewportControllerArray;

	// [0] Perspective, [1] Top, [2] Side, [3] Bottom
	// 소유권은 ViewportClientArray
	FEditorViewportClient* SceneViewportClients[4] = { nullptr, nullptr, nullptr, nullptr };
};