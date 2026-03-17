#pragma once

#include "Core/ViewportClient.h"
#include "Gizmo/Gizmo.h"
#include "Picking/Picker.h"

class CEditorUI;
class CWindow;
class FFrustum;
struct FRenderCommandQueue;

class CEditorViewportClient : public IViewportClient
{
public:
	CEditorViewportClient(CEditorUI& InEditorUI, CWindow* InMainWindow);

	void Attach(CCore* Core, CRenderer* Renderer) override;
	void Detach(CCore* Core, CRenderer* Renderer) override;
	void Tick(CCore* Core, float DeltaTime) override;
	void HandleMessage(CCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam) override;
	void BuildRenderCommands(CCore* Core, UScene* Scene, const FFrustum& Frustum, FRenderCommandQueue& OutQueue) const override;

private:
	CEditorUI& EditorUI;
	CWindow* MainWindow = nullptr;
	CPicker Picker;
	mutable CGizmo Gizmo;
};
