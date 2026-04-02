#pragma once
#include "EngineAPI.h"
#include "UI/SWindow.h"
#include "Core/ViewportContext.h"
#include <memory>

class SViewportWindow : public SWindow
{
	FViewportContext* ViewportContext;

public:
	explicit SViewportWindow(FRect InRect, FViewportContext* InViewportContext);
	~SViewportWindow() override;
	FViewportContext* GetViewportContext() const { return ViewportContext; }

	virtual void Tick(float DeltaTime) override;
	virtual void Render() override;
	virtual void Draw() override;
	virtual void OnResize() override;
	virtual bool HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam) override;
};

