#pragma once
#include "Core/FEngine.h"
#include "UI/EditorUI.h"

class FEditorEngine : public FEngine
{
public:
	FEditorEngine() = default;

	bool Initialize(HINSTANCE hInstance);

protected:
	void PreInitialize() override;
	void PostInitialize() override;

private:
	CEditorUI EditorUI;
};
