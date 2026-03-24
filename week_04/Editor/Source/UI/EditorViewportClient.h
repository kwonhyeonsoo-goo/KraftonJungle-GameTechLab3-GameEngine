#pragma once

#include "Core/ViewportClient.h"
#include "Gizmo/Gizmo.h"
#include "Picking/Picker.h"

class CEditorUI;
class CWindow;
class FFrustum;
struct FRenderCommandQueue;

enum class ERenderMode
{
	Lighting,
	NoLighting,
	Wireframe,
};

class CEditorViewportClient : public IViewportClient
{
public:
	CEditorViewportClient(CEditorUI& InEditorUI, CWindow* InMainWindow);

	void Attach(CCore* Core, CRenderer* Renderer) override;
	void Detach(CCore* Core, CRenderer* Renderer) override;
	void Tick(CCore* Core, float DeltaTime) override;
	void HandleMessage(CCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam) override;
	EGizmoMode GetGizmoMode() const { return Gizmo.GetMode(); }
	void SetGizmoMode(EGizmoMode InMode) { Gizmo.SetMode(InMode); }
	ERenderMode GetRenderMode() { return RenderMode; }
	void SetRenderMode(ERenderMode InRenderMode) { RenderMode = InRenderMode; }

	void HandleFileDoubleClick(const FString& FilePath) override;
	void BuildRenderCommands(CCore* Core, UScene* Scene,
		const FFrustum& Frustum, FRenderCommandQueue& OutQueue) override;
protected:
	virtual FRenderCommand BuildRenderCommand(UPrimitiveComponent* PrimitiveComponent) const override;

private:
	CEditorUI& EditorUI;
	CWindow* MainWindow = nullptr;
	CPicker Picker;
	mutable CGizmo Gizmo;

	ERenderMode RenderMode = ERenderMode::Lighting;
	const FString WireframeMaterialName = "M_Wireframe";
	std::shared_ptr<FMaterial> WireFrameMaterial = nullptr;
};
