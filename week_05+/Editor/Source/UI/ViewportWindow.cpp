#include "ViewportWindow.h"
#include "Core/FEngine.h"
#include "imgui.h"

SViewportWindow::SViewportWindow(FRect InRect, FViewportContext* InViewportContext)
	: SWindow(InRect), ViewportContext(InViewportContext)
{
	InViewportContext->GetViewportClient()->SetViewportWindow(this);
}

SViewportWindow::~SViewportWindow()
{
	if (ViewportContext)
	{
		ViewportContext->Cleanup();
	}
	delete ViewportContext;
}

void SViewportWindow::Tick(float DeltaTime)
{
	if (ViewportContext && GEngine)
	{
		ViewportContext->Tick(DeltaTime);
	}
}

void SViewportWindow::Render()
{
	if (ViewportContext && GEngine)
	{
		ViewportContext->Render(GEngine->GetCore(), GEngine->GetCommandQueue());
	}
}

void SViewportWindow::Draw()
{
	ViewportContext->GetViewportClient()->DrawUI();
}

void SViewportWindow::OnResize()
{
	if (!ViewportContext)
	{
		return;
	}

	ViewportContext->SetRect(GetRect());
}

bool SViewportWindow::HandleMessage(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (!ViewportContext)
	{
		return false;
	}

	return ViewportContext->HandleMessage(Hwnd, Msg, WParam, LParam);
}
