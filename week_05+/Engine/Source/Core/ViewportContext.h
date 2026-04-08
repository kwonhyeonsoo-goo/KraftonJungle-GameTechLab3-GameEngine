#pragma once

#include "CoreMinimal.h"
#include "Viewport.h"
#include "ViewportClient.h"
#include <memory>

class FCore;
class AActor;
class UWorld;
struct FRenderCommandQueue;

struct ENGINE_API FViewportContext
{
	FViewport* Viewport;
	FViewportClient* ViewportClient;
	bool bEnabled = true;
	bool bAcceptsInput = true;
	bool bActive = false;
	bool bCapturing = false;

	FViewportContext() = default;
	FViewportContext(FViewport* InViewport, FViewportClient* InViewportClient);
	FViewportContext(FViewportContext&& Other) noexcept;
	FViewportContext& operator=(FViewportContext&& Other) noexcept;
	~FViewportContext();

	bool IsValid() const;
	bool IsEnabled() const;
	void SetEnabled(bool bInEnabled);
	bool AcceptsInput() const;
	void SetAcceptsInput(bool bInAcceptsInput);
	FViewport* GetViewport() const;
	FViewportClient* GetViewportClient() const;
	bool IsActive() const { return bActive; }
	bool IsCapturing() const { return bCapturing; }
	bool IsHovered() const { return Viewport && Viewport->IsHovered(); }
	void SetActive(bool bInActive);
	void SetCapturing(bool bInCapturing);
	void ApplyLayout(const FRect& InRect);
	void SetRect(FRect InRect);
	bool ContainsPoint(int32 WindowMouseX, int32 WindowMouseY) const;
	void UpdateInteractionState(int32 WindowMouseX, int32 WindowMouseY, bool bInFocused, bool bInCapturing);
	void Initialize(FInputManager* InInputManager, FEnhancedInputManager* InEnhancedInput);
	bool IsInteractive() const;
	void Tick(float DeltaTime);
	bool HandleMessage(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);
	void PrepareView(FRenderCommandQueue& CommandQueue, TArray<AActor*>& OutActors) const;
	void Render(FCore* Core, FRenderCommandQueue& CommandQueue);
	void Cleanup();
};
