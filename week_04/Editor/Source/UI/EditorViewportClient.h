#pragma once

#include "Core/ViewportClient.h"
#include "Picking/Picker.h"

class CEditorUI;
class CWindow;

class CEditorViewportClient : public IViewportClient
{
public:
	CEditorViewportClient(CEditorUI& InEditorUI, CWindow* InMainWindow);

	void Attach(CCore* Core, CRenderer* Renderer) override;
	void Detach(CCore* Core, CRenderer* Renderer) override;
	void Tick(CCore* Core, float DeltaTime) override;
	void HandleMessage(CCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam) override;

private:
	CEditorUI& EditorUI;
	CWindow* MainWindow = nullptr;
	CPicker Picker;
};
