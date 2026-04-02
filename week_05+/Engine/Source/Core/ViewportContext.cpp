#include "ViewportContext.h"
#include "Core.h"
#include "Renderer/Renderer.h"
#include "World/World.h"
#include <windowsx.h>

namespace
{
	bool IsMouseMessage(UINT Msg)
	{
		switch (Msg)
		{
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
			return true;
		default:
			return false;
		}
	}
}

FViewportContext::FViewportContext() = default;

FViewportContext::FViewportContext(FViewport* InViewport, FViewportClient* InViewportClient)
	: Viewport(InViewport)
	, ViewportClient(InViewportClient)
{
}

FViewportContext::FViewportContext(FViewportContext&& Other) noexcept = default;

FViewportContext& FViewportContext::operator=(FViewportContext&& Other) noexcept = default;

FViewportContext::~FViewportContext() 
{
	if(ViewportClient)
	{
		delete ViewportClient;
	}

	if (Viewport)
	{
		delete Viewport;
	}
}

bool FViewportContext::IsValid() const
{
	return Viewport != nullptr && ViewportClient != nullptr;
}

bool FViewportContext::IsEnabled() const
{
	return bEnabled && IsValid();
}

void FViewportContext::SetEnabled(bool bInEnabled)
{
	bEnabled = bInEnabled;
	if (!bEnabled)
	{
		SetActive(false);
		SetCapturing(false);
		if (Viewport)
		{
			Viewport->SetHovered(false);
			Viewport->SetFocused(false);
		}
	}
}

bool FViewportContext::AcceptsInput() const
{
	return bAcceptsInput && IsEnabled();
}

FViewport* FViewportContext::GetViewport() const
{
	return Viewport;
}

FViewportClient* FViewportContext::GetViewportClient() const
{
	return ViewportClient;
}

void FViewportContext::SetAcceptsInput(bool bInAcceptsInput)
{
	bAcceptsInput = bInAcceptsInput;
	if (!bAcceptsInput)
	{
		SetActive(false);
	}
}

void FViewportContext::SetActive(bool bInActive)
{
	bActive = bInActive && AcceptsInput();
}

void FViewportContext::SetCapturing(bool bInCapturing)
{
	bCapturing = bInCapturing && AcceptsInput();
}

void FViewportContext::ApplyLayout(const FRect& InRect)
{
	SetRect(InRect);
	SetEnabled(InRect.Size.X > 0.0f && InRect.Size.Y > 0.0f);
}

void FViewportContext::SetRect(FRect InRect)
{
	if (!Viewport)
	{
		return;
	}

	Viewport->SetRect(InRect);
	if (ViewportClient)
	{
		ViewportClient->SetViewportRect(InRect);
	}
}

bool FViewportContext::ContainsPoint(int32 WindowMouseX, int32 WindowMouseY) const
{
	return Viewport && Viewport->ContainsPoint(WindowMouseX, WindowMouseY);
}

void FViewportContext::UpdateInteractionState(int32 WindowMouseX, int32 WindowMouseY, bool bInFocused, bool bInCapturing)
{
	if (!Viewport)
	{
		return;
	}

	const bool bHovered = ContainsPoint(WindowMouseX, WindowMouseY);
	Viewport->SetHovered(bHovered);
	Viewport->SetFocused(bInFocused);
	SetCapturing(bInCapturing);
}

void FViewportContext::Initialize(FCore* Core, FInputManager* InInputManager, FEnhancedInputManager* InEnhancedInput)
{
	if (ViewportClient)
	{
		ViewportClient->Initialize(InInputManager, InEnhancedInput);
		ViewportClient->Attach(Core);
	}
}

bool FViewportContext::IsInteractive() const
{
	return AcceptsInput() && Viewport && Viewport->IsVisible() && ViewportClient != nullptr;
}

void FViewportContext::Tick(FCore* Core, float DeltaTime)
{
	if (!Core || !IsEnabled() || ViewportClient == nullptr)
	{
		return;
	}

	if (!bActive && !bCapturing)
	{
		return;
	}

	ViewportClient->Tick(DeltaTime);
	ViewportClient->ProcessCameraInput(Core, DeltaTime);
}

bool FViewportContext::HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (!Core || !AcceptsInput() || ViewportClient == nullptr || Viewport == nullptr)
	{
		return false;
	}

	const bool bMouseMessage = IsMouseMessage(Msg);
	if (bMouseMessage)
	{
		int32 LocalMouseX = 0;
		int32 LocalMouseY = 0;
		int32 ViewWidth = 0;
		int32 ViewHeight = 0;
		const bool bHasLocalMousePosition = Viewport->GetMousePositionInViewport(static_cast<int32>(GET_X_LPARAM(LParam)), static_cast<int32>(GET_Y_LPARAM(LParam)), LocalMouseX, LocalMouseY, ViewWidth, ViewHeight);
		if (!bHasLocalMousePosition && Msg != WM_MOUSEWHEEL && Msg != WM_MOUSEHWHEEL)
		{
			return false;
		}

		if (bHasLocalMousePosition)
		{
			ViewportClient->SetViewportInputState(LocalMouseX, LocalMouseY, Viewport->GetRect());
		}
	}

	ViewportClient->HandleMessage(Core, Hwnd, Msg, WParam, LParam);
	return true;
}

UWorld* FViewportContext::ResolveWorld(FCore* Core) const
{
	if (!Core || !ViewportClient)
	{
		return nullptr;
	}

	return ViewportClient->ResolveWorld(Core);
}

void FViewportContext::PrepareView(FRenderCommandQueue& CommandQueue, TArray<AActor*>& OutActors) const
{
	CommandQueue.Clear();
	if (!GRenderer || !ViewportClient)
	{
		return;
	}

	CommandQueue.Reserve(GRenderer->GetPrevCommandCount());
	ViewportClient->BuildRenderCommands(OutActors, CommandQueue);
}

void FViewportContext::Render(FCore* Core, FRenderCommandQueue& CommandQueue)
{
	if (!Core || !IsEnabled() || !GRenderer || !Viewport || !ViewportClient)
	{
		return;
	}

	UWorld* World = ResolveWorld(Core);
	if (!World)
	{
		return;
	}

	CommandQueue.Clear();
	TArray<AActor*> Actors = World->GetAllActors();
	PrepareView(CommandQueue, Actors);

	D3D11_VIEWPORT D3DViewport = Viewport->GetD3D11Viewport();
	GRenderer->SetViewport(&D3DViewport);
	GRenderer->ExecuteCommandQueue(CommandQueue);
	ViewportClient->PostRender(Core, GRenderer);
	Core->GetDebugDrawManager().Flush(GRenderer, ViewportClient->GetShowFlags(), World);
}

void FViewportContext::Cleanup()
{
	SetActive(false);
	SetCapturing(false);

	if (ViewportClient)
	{
		ViewportClient->Detach();
		ViewportClient->Cleanup();
	}
}
