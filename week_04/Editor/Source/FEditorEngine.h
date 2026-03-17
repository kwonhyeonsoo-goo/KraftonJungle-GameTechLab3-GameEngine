#pragma once
#include "Core/FEngine.h"
#include "UI/EditorUI.h"
#include "Pawn/EditorCameraPawn.h"
#include "Controller/EditorViewportController.h"
class FEditorEngine : public FEngine
{
public:
	FEditorEngine() = default;

	bool Initialize(HINSTANCE hInstance);
	void Tick(float DeltaTime) override;
protected:
	void PreInitialize() override;
	void PostInitialize() override;

private:
	CEditorUI EditorUI;
	AEditorCameraPawn* EditorPawn = nullptr;
	CEditorViewportController ViewportController;
};
