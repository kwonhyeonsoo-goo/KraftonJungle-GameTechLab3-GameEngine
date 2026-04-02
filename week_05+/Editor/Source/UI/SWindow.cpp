#include "SWindow.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <stdexcept>
#include <windowsx.h>
#include "CoreMinimal.h"
#include "Windows.h"
#include "imgui.h"
#include "FEditorEngine.h"

namespace
{
	void RequestEditorLayoutSave()
	{
		if (FEditorEngine* EditorEngine = dynamic_cast<FEditorEngine*>(GEngine))
		{
			EditorEngine->SaveEditorSettings();
		}
	}

	bool IsMouseMessage(UINT Msg)
	{
		switch (Msg)
		{
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
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

	ImGuiWindowFlags GetSplitterOverlayWindowFlags()
	{
		return
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoBackground;
	}

	ImVec2 ToScreenPosition(const FPoint& Position)
	{
		const ImGuiViewport* MainViewport = ImGui::GetMainViewport();
		if (MainViewport == nullptr)
		{
			return ImVec2(Position.X, Position.Y);
		}

		return ImVec2(MainViewport->Pos.x + Position.X, MainViewport->Pos.y + Position.Y);
	}

	ImU32 GetSplitterColor(bool bHovered, bool bActive)
	{
		if (bActive)
		{
			return IM_COL32(110, 170, 255, 220);
		}

		if (bHovered)
		{
			return IM_COL32(150, 150, 150, 200);
		}

		return IM_COL32(90, 90, 90, 160);
	}
}

void SWindow::SetRect(const FRect& InRect)
{
	Rect = InRect;
	OnResize();
}

void SWindow::ReplaceSide(SWindow* OldSide, SWindow* NewSide)
{
	(void)OldSide;
	(void)NewSide;
	throw std::runtime_error("This window does not support replacing child windows.");
}

SWindow* SWindow::Merge(SWindow* InWindowToKeep)
{
	(void)InWindowToKeep;
	throw std::runtime_error("This window does not support merging child windows.");
}

bool SWindow::ISHover(FPoint coord) const
{
	return coord.X >= Rect.Position.X && coord.X <= Rect.Position.X + Rect.Size.X &&
		coord.Y >= Rect.Position.Y && coord.Y <= Rect.Position.Y + Rect.Size.Y;
}

SWindow* SWindow::GetWindow(FPoint coord)
{
	if (ISHover(coord))
	{
		return this;
	}

	return nullptr;
}

SSplitter* SWindow::Split(SWindow* InNewWindow, SplitDirection InDirection, SplitOption InSplitOption)
{
	if (InNewWindow == nullptr)
		return nullptr;

	SSplitter* NewSplitter = nullptr;

	if (InDirection == SplitDirection::Horizontal)
	{
		NewSplitter = new SSplitterH(GetRect());
	}
	else if (InDirection == SplitDirection::Vertical)
	{
		NewSplitter = new SSplitterV(GetRect());
	}

	if (!NewSplitter)
		return nullptr;

	NewSplitter->SetParent(Parent);
	if (Parent)
	{
		Parent->ReplaceSide(this, NewSplitter);
	}
	else if (FEditorEngine* EditorEngine = dynamic_cast<FEditorEngine*>(GEngine))
	{
		EditorEngine->GetWindowManager().ReplaceWindow(this, NewSplitter);
	}

	if (InSplitOption == SplitOption::LT)
	{
		NewSplitter->SetSideLT(this);
		NewSplitter->SetSideRB(InNewWindow);
	}
	else if (InSplitOption == SplitOption::RB)
	{
		NewSplitter->SetSideLT(InNewWindow);
		NewSplitter->SetSideRB(this);
	}

	return NewSplitter;
}

SSplitterC* SWindow::Split4(SWindow* InNewWindow1, SWindow* InNewWindow2, SWindow* InNewWindow3, SplitOption InSplitOption)
{
	if (InNewWindow1 == nullptr || InNewWindow2 == nullptr || InNewWindow3 == nullptr)
	{
		return nullptr;
	}

	SSplitterC* NewSplitter = new SSplitterC(GetRect());
	if (NewSplitter == nullptr)
	{
		return nullptr;
	}

	NewSplitter->SetParent(Parent);
	if (Parent)
	{
		Parent->ReplaceSide(this, NewSplitter);
	}
	else if (FEditorEngine* EditorEngine = dynamic_cast<FEditorEngine*>(GEngine))
	{
		EditorEngine->GetWindowManager().ReplaceWindow(this, NewSplitter);
	}

	switch (InSplitOption)
	{
	case SplitOption::LT:
		NewSplitter->SetSideLT(this);
		NewSplitter->SetSideRT(InNewWindow1);
		NewSplitter->SetSideLB(InNewWindow2);
		NewSplitter->SetSideRB(InNewWindow3);
		break;
	case SplitOption::RT:
		NewSplitter->SetSideLT(InNewWindow1);
		NewSplitter->SetSideRT(this);
		NewSplitter->SetSideLB(InNewWindow2);
		NewSplitter->SetSideRB(InNewWindow3);
		break;
	case SplitOption::LB:
		NewSplitter->SetSideLT(InNewWindow1);
		NewSplitter->SetSideRT(InNewWindow2);
		NewSplitter->SetSideLB(this);
		NewSplitter->SetSideRB(InNewWindow3);
		break;
	case SplitOption::RB:
		NewSplitter->SetSideLT(InNewWindow1);
		NewSplitter->SetSideRT(InNewWindow2);
		NewSplitter->SetSideLB(InNewWindow3);
		NewSplitter->SetSideRB(this);
		break;
	default:
		delete NewSplitter;
		return nullptr;
	}

	return NewSplitter;
}

void SSplitter::SetSideLT(SWindow* InSideLT)
{
	SideLT = InSideLT;
	if (InSideLT)
	{
		InSideLT->SetParent(this);
		InSideLT->SetRect(GetSideLTRect());
	}
}

void SSplitter::SetSideRB(SWindow* InSideRB)
{
	SideRB = InSideRB;
	if (InSideRB)
	{
		InSideRB->SetParent(this);
		InSideRB->SetRect(GetSideRBRect());
	}
}

void SSplitter::OnResize()
{
	if (SideLT)
	{
		SideLT->SetRect(GetSideLTRect());
	}
	if (SideRB)
	{
		SideRB->SetRect(GetSideRBRect());
	}
}

void SSplitter::ReplaceSide(SWindow* OldSide, SWindow* NewSide)
{
	if (OldSide == SideLT)
	{
		SetSideLT(NewSide);
		return;
	}

	if (OldSide == SideRB)
	{
		SetSideRB(NewSide);
		return;
	}

	throw std::runtime_error("OldSide is not part of this splitter.");
}

SWindow* SSplitter::Merge(SWindow* InWindowToKeep)
{
	if (InWindowToKeep != SideLT && InWindowToKeep != SideRB)
	{
		throw std::runtime_error("InWindowToKeep is not part of this splitter.");
	}

	FEditorEngine* EditorEngine = dynamic_cast<FEditorEngine*>(GEngine);
	SWindow* ParentWindow = GetParent();
	SWindow* WindowToKeep = InWindowToKeep;
	SWindow* WindowToRemove = (WindowToKeep == SideLT) ? SideRB : SideLT;

	if (WindowToKeep == SideLT)
	{
		SideLT = nullptr;
	}
	else
	{
		SideRB = nullptr;
	}

	if (WindowToRemove == SideLT)
	{
		SideLT = nullptr;
	}
	else if (WindowToRemove == SideRB)
	{
		SideRB = nullptr;
	}

	if (EditorEngine)
	{
		EditorEngine->GetWindowManager().QueueDestroyWindow(WindowToRemove);
	}
	else
	{
		delete WindowToRemove;
	}

	if (ParentWindow)
	{
		ParentWindow->ReplaceSide(this, WindowToKeep);
	}
	else
	{
		WindowToKeep->SetParent(nullptr);
		WindowToKeep->SetRect(GetRect());
	}

	if (EditorEngine)
	{
		EditorEngine->GetWindowManager().ReplaceWindow(this, WindowToKeep);
		EditorEngine->GetWindowManager().QueueDestroyWindow(this);
	}

	return WindowToKeep;
}

SSplitter::SSplitter(FRect InRect, SWindow* InSideLT, SWindow* InSideRB, float InSplitRatio)
	: SWindow(InRect), SideLT(InSideLT), SideRB(InSideRB), SplitRatio(InSplitRatio)
{
}

SSplitter::~SSplitter()
{
	if (SideLT)
	{
		delete SideLT;
		SideLT = nullptr;
	}
	if (SideRB)
	{
		delete SideRB;
		SideRB = nullptr;
	}
}

SWindow* SSplitter::GetWindow(FPoint coord)
{
	if (SideLT && SideLT->ISHover(coord))
	{
		return SideLT->GetWindow(coord);
	}
	else if (SideRB && SideRB->ISHover(coord))
	{
		return SideRB->GetWindow(coord);
	}
	return nullptr;
}

float SSplitter::ClampSplitRatio(float InSplitRatio) const
{
	const float AvailableAxis = GetAvailablePrimaryAxisSize();
	if (AvailableAxis <= 0.0f)
	{
		return 0.5f;
	}

	const float ClampedMinPaneSize = (std::max)(0.0f, MinPaneSize);
	float MinRatio = ClampedMinPaneSize / AvailableAxis;
	float MaxRatio = 1.0f - MinRatio;

	if (MinRatio >= MaxRatio)
	{
		return 0.5f;
	}

	return (std::clamp)(InSplitRatio, MinRatio, MaxRatio);
}

float SSplitter::GetAvailablePrimaryAxisSize() const
{
	return (std::max)(0.0f, GetPrimaryAxisSize() - SplitterThickness);
}

void SSplitter::InitializeSplitterLayout()
{
	SplitRatio = ClampSplitRatio(SplitRatio);

	if (SideLT)
	{
		SideLT->SetParent(this);
		SideLT->SetRect(GetSideLTRect());
	}

	if (SideRB)
	{
		SideRB->SetParent(this);
		SideRB->SetRect(GetSideRBRect());
	}
}

void SSplitter::SetSplitRatio(float InSplitRatio)
{
	const float ClampedSplitRatio = ClampSplitRatio(InSplitRatio);
	if (SplitRatio == ClampedSplitRatio)
	{
		return;
	}

	SplitRatio = ClampedSplitRatio;
	OnResize();
	RequestEditorLayoutSave();
}

void SSplitter::Tick(float DeltaTime)
{
	if (SideLT)
	{
		SideLT->Tick(DeltaTime);
	}
	if (SideRB)
	{
		SideRB->Tick(DeltaTime);
	}
}

void SSplitter::Render()
{
	if (SideLT)
	{
		SideLT->Render();
	}
	if (SideRB)
	{
		SideRB->Render();
	}
}

void SSplitter::Draw()
{
	const float ClampedSplitRatio = ClampSplitRatio(SplitRatio);
	if (SplitRatio != ClampedSplitRatio)
	{
		SplitRatio = ClampedSplitRatio;
		OnResize();
	}

	if (SideLT)
	{
		SideLT->Draw();
	}
	if (SideRB)
	{
		SideRB->Draw();
	}

	DrawSplitterHandle();
}

bool SSplitter::HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (bDragging && IsMouseMessage(Msg))
	{
		return true;
	}

	if (IsMouseMessage(Msg))
	{
		const FPoint MousePoint(static_cast<float>(GET_X_LPARAM(LParam)), static_cast<float>(GET_Y_LPARAM(LParam)));
		if (SideLT && SideLT->ISHover(MousePoint))
		{
			return SideLT->HandleMessage(Core, Hwnd, Msg, WParam, LParam);
		}

		if (SideRB && SideRB->ISHover(MousePoint))
		{
			return SideRB->HandleMessage(Core, Hwnd, Msg, WParam, LParam);
		}

		return false;
	}

	if (SideLT && SideLT->HandleMessage(Core, Hwnd, Msg, WParam, LParam))
	{
		return true;
	}

	if (SideRB && SideRB->HandleMessage(Core, Hwnd, Msg, WParam, LParam))
	{
		return true;
	}

	return false;
}

void SSplitter::DrawSplitterHandle()
{
	const FRect SplitterRect = GetSplitterRect();
	if (SplitterRect.Size.X <= 0.0f || SplitterRect.Size.Y <= 0.0f)
	{
		bDragging = false;
		return;
	}

	char OverlayName[64];
	std::snprintf(OverlayName, sizeof(OverlayName), "##SplitterOverlay_%p", static_cast<void*>(this));

	ImGui::SetNextWindowPos(ToScreenPosition(SplitterRect.Position), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(SplitterRect.Size.X, SplitterRect.Size.Y), ImGuiCond_Always);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	if (ImGui::Begin(OverlayName, nullptr, GetSplitterOverlayWindowFlags()))
	{
		ImGui::PushID(this);
		ImGui::InvisibleButton("##Handle", ImVec2(SplitterRect.Size.X, SplitterRect.Size.Y));

		const bool bHovered = ImGui::IsItemHovered();
		const bool bActive = ImGui::IsItemActive();
		bDragging = bActive;

		if (bHovered || bActive)
		{
			ImGui::SetMouseCursor(GetSplitterMouseCursor());
		}

		if (bActive)
		{
			const float AvailableAxis = GetAvailablePrimaryAxisSize();
			if (AvailableAxis > 0.0f)
			{
				// SplitRatio = FirstPaneSize / AvailableAxis.
				// Converting drag delta from pixels to ratio is just delta / AvailableAxis.
				const float DeltaRatio = GetMouseDeltaForSplit() / AvailableAxis;
				SetSplitRatio(SplitRatio + DeltaRatio);
			}
			else
			{
				SetSplitRatio(0.5f);
			}
		}

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		DrawList->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), GetSplitterColor(bHovered, bActive));

		ImGui::PopID();
	}

	ImGui::End();
	ImGui::PopStyleVar(2);
}

FRect SSplitterV::GetSideLTRect()
{
	const float AvailableHeight = (std::max)(0.0f, Rect.Size.Y - SplitterThickness);
	const float Height = AvailableHeight * SplitRatio;
	return FRect({ Rect.Position.X, Rect.Position.Y }, { Rect.Size.X, Height });
}

FRect SSplitterV::GetSideRBRect()
{
	const float AvailableHeight = (std::max)(0.0f, Rect.Size.Y - SplitterThickness);
	const float TopHeight = AvailableHeight * SplitRatio;
	const float BottomHeight = AvailableHeight - TopHeight;
	return FRect(
		{ Rect.Position.X, Rect.Position.Y + TopHeight + SplitterThickness },
		{ Rect.Size.X, BottomHeight });
}

FRect SSplitterH::GetSideLTRect()
{
	const float AvailableWidth = (std::max)(0.0f, Rect.Size.X - SplitterThickness);
	const float Width = AvailableWidth * SplitRatio;
	return FRect({ Rect.Position.X, Rect.Position.Y }, { Width, Rect.Size.Y });
}

FRect SSplitterH::GetSideRBRect()
{
	const float AvailableWidth = (std::max)(0.0f, Rect.Size.X - SplitterThickness);
	const float LeftWidth = AvailableWidth * SplitRatio;
	const float RightWidth = AvailableWidth - LeftWidth;
	return FRect(
		{ Rect.Position.X + LeftWidth + SplitterThickness, Rect.Position.Y },
		{ RightWidth, Rect.Size.Y });
}

float SSplitterV::GetPrimaryAxisSize() const
{
	return Rect.Size.Y;
}

float SSplitterV::GetMouseDeltaForSplit() const
{
	return ImGui::GetIO().MouseDelta.y;
}

ImGuiMouseCursor SSplitterV::GetSplitterMouseCursor() const
{
	return ImGuiMouseCursor_ResizeNS;
}

FRect SSplitterV::GetSplitterRect() const
{
	const float AvailableHeight = (std::max)(0.0f, Rect.Size.Y - SplitterThickness);
	const float TopHeight = AvailableHeight * SplitRatio;
	return FRect(
		{ Rect.Position.X, Rect.Position.Y + TopHeight },
		{ Rect.Size.X, SplitterThickness });
}

void SSplitterV::Draw()
{
	SSplitter::Draw();
}

float SSplitterH::GetPrimaryAxisSize() const
{
	return Rect.Size.X;
}

float SSplitterH::GetMouseDeltaForSplit() const
{
	return ImGui::GetIO().MouseDelta.x;
}

ImGuiMouseCursor SSplitterH::GetSplitterMouseCursor() const
{
	return ImGuiMouseCursor_ResizeEW;
}

FRect SSplitterH::GetSplitterRect() const
{
	const float AvailableWidth = (std::max)(0.0f, Rect.Size.X - SplitterThickness);
	const float LeftWidth = AvailableWidth * SplitRatio;
	return FRect(
		{ Rect.Position.X + LeftWidth, Rect.Position.Y },
		{ SplitterThickness, Rect.Size.Y });
}

void SSplitterH::Draw()
{
	SSplitter::Draw();
}

float SSplitterC::ClampHorizontalRatio(float InSplitRatio) const
{
	const float AvailableWidth = GetAvailableWidth();
	if (AvailableWidth <= 0.0f)
	{
		return 0.5f;
	}

	const float MinRatio = (std::max)(0.0f, MinPaneSize) / AvailableWidth;
	const float MaxRatio = 1.0f - MinRatio;
	if (MinRatio >= MaxRatio)
	{
		return 0.5f;
	}

	return (std::clamp)(InSplitRatio, MinRatio, MaxRatio);
}

float SSplitterC::ClampVerticalRatio(float InSplitRatio) const
{
	const float AvailableHeight = GetAvailableHeight();
	if (AvailableHeight <= 0.0f)
	{
		return 0.5f;
	}

	const float MinRatio = (std::max)(0.0f, MinPaneSize) / AvailableHeight;
	const float MaxRatio = 1.0f - MinRatio;
	if (MinRatio >= MaxRatio)
	{
		return 0.5f;
	}

	return (std::clamp)(InSplitRatio, MinRatio, MaxRatio);
}

float SSplitterC::GetAvailableWidth() const
{
	return (std::max)(0.0f, Rect.Size.X - SplitterThickness);
}

float SSplitterC::GetAvailableHeight() const
{
	return (std::max)(0.0f, Rect.Size.Y - SplitterThickness);
}

void SSplitterC::SetSideLT(SWindow* InSideLT)
{
	SideLT = InSideLT;
	if (InSideLT)
	{
		InSideLT->SetParent(this);
		InSideLT->SetRect(GetSideLTRect());
	}
}

void SSplitterC::SetSideLB(SWindow* InSideLB)
{
	SideLB = InSideLB;
	if (InSideLB)
	{
		InSideLB->SetParent(this);
		InSideLB->SetRect(GetSideLBRect());
	}
}

void SSplitterC::SetSideRT(SWindow* InSideRT)
{
	SideRT = InSideRT;
	if (InSideRT)
	{
		InSideRT->SetParent(this);
		InSideRT->SetRect(GetSideRTRect());
	}
}

void SSplitterC::SetSideRB(SWindow* InSideRB)
{
	SideRB = InSideRB;
	if (InSideRB)
	{
		InSideRB->SetParent(this);
		InSideRB->SetRect(GetSideRBRect());
	}
}

FRect SSplitterC::GetSideLTRect()
{
	const float LeftWidth = GetAvailableWidth() * SplitRatioHorizontal;
	const float TopHeight = GetAvailableHeight() * SplitRatioVertical;
	return FRect(Rect.Position, { LeftWidth, TopHeight });
}

FRect SSplitterC::GetSideLBRect()
{
	const float LeftWidth = GetAvailableWidth() * SplitRatioHorizontal;
	const float TopHeight = GetAvailableHeight() * SplitRatioVertical;
	const float BottomHeight = GetAvailableHeight() - TopHeight;
	return FRect(
		{ Rect.Position.X, Rect.Position.Y + TopHeight + SplitterThickness },
		{ LeftWidth, BottomHeight });
}

FRect SSplitterC::GetSideRTRect()
{
	const float LeftWidth = GetAvailableWidth() * SplitRatioHorizontal;
	const float RightWidth = GetAvailableWidth() - LeftWidth;
	const float TopHeight = GetAvailableHeight() * SplitRatioVertical;
	return FRect(
		{ Rect.Position.X + LeftWidth + SplitterThickness, Rect.Position.Y },
		{ RightWidth, TopHeight });
}

FRect SSplitterC::GetSideRBRect()
{
	const float LeftWidth = GetAvailableWidth() * SplitRatioHorizontal;
	const float RightWidth = GetAvailableWidth() - LeftWidth;
	const float TopHeight = GetAvailableHeight() * SplitRatioVertical;
	const float BottomHeight = GetAvailableHeight() - TopHeight;
	return FRect(
		{ Rect.Position.X + LeftWidth + SplitterThickness, Rect.Position.Y + TopHeight + SplitterThickness },
		{ RightWidth, BottomHeight });
}

void SSplitterC::OnResize()
{
	SplitRatioHorizontal = ClampHorizontalRatio(SplitRatioHorizontal);
	SplitRatioVertical = ClampVerticalRatio(SplitRatioVertical);

	if (SideLT)
	{
		SideLT->SetRect(GetSideLTRect());
	}
	if (SideLB)
	{
		SideLB->SetRect(GetSideLBRect());
	}
	if (SideRT)
	{
		SideRT->SetRect(GetSideRTRect());
	}
	if (SideRB)
	{
		SideRB->SetRect(GetSideRBRect());
	}
}

void SSplitterC::ReplaceSide(SWindow* OldSide, SWindow* NewSide)
{
	if (OldSide == SideLT)
	{
		SetSideLT(NewSide);
		return;
	}
	if (OldSide == SideLB)
	{
		SetSideLB(NewSide);
		return;
	}
	if (OldSide == SideRT)
	{
		SetSideRT(NewSide);
		return;
	}
	if (OldSide == SideRB)
	{
		SetSideRB(NewSide);
		return;
	}

	throw std::runtime_error("OldSide is not part of this quad splitter.");
}

SWindow* SSplitterC::Merge(SWindow* InWindowToKeep)
{
	if (InWindowToKeep != SideLT && InWindowToKeep != SideLB && InWindowToKeep != SideRT && InWindowToKeep != SideRB)
	{
		throw std::runtime_error("InWindowToKeep is not part of this quad splitter.");
	}

	FEditorEngine* EditorEngine = dynamic_cast<FEditorEngine*>(GEngine);
	SWindow* ParentWindow = GetParent();
	SWindow* WindowToKeep = InWindowToKeep;
	SWindow* WindowsToRemove[] = { SideLT, SideLB, SideRT, SideRB };

	SideLT = nullptr;
	SideLB = nullptr;
	SideRT = nullptr;
	SideRB = nullptr;

	for (SWindow* WindowToRemove : WindowsToRemove)
	{
		if (WindowToRemove && WindowToRemove != WindowToKeep)
		{
			if (EditorEngine)
			{
				EditorEngine->GetWindowManager().QueueDestroyWindow(WindowToRemove);
			}
			else
			{
				delete WindowToRemove;
			}
		}
	}

	if (ParentWindow)
	{
		ParentWindow->ReplaceSide(this, WindowToKeep);
	}
	else
	{
		WindowToKeep->SetParent(nullptr);
		WindowToKeep->SetRect(GetRect());
	}

	if (EditorEngine)
	{
		EditorEngine->GetWindowManager().ReplaceWindow(this, WindowToKeep);
		EditorEngine->GetWindowManager().QueueDestroyWindow(this);
	}

	return WindowToKeep;
}

SSplitterC::SSplitterC(FRect InRect, SWindow* InSideLT, SWindow* InSideLB, SWindow* InSideRT, SWindow* InSideRB, float InSplitRatioHorizontal, float InSplitRatioVertical)
	: SWindow(InRect)
	, SideLT(InSideLT)
	, SideLB(InSideLB)
	, SideRT(InSideRT)
	, SideRB(InSideRB)
	, SplitRatioVertical(InSplitRatioVertical)
	, SplitRatioHorizontal(InSplitRatioHorizontal)
{
	OnResize();

	if (SideLT)
	{
		SideLT->SetParent(this);
	}
	if (SideLB)
	{
		SideLB->SetParent(this);
	}
	if (SideRT)
	{
		SideRT->SetParent(this);
	}
	if (SideRB)
	{
		SideRB->SetParent(this);
	}
}

SSplitterC::~SSplitterC()
{
	delete SideLT;
	delete SideLB;
	delete SideRT;
	delete SideRB;
	SideLT = nullptr;
	SideLB = nullptr;
	SideRT = nullptr;
	SideRB = nullptr;
}

void SSplitterC::SetSplitRatioVertical(float InSplitRatio)
{
	const float ClampedSplitRatio = ClampVerticalRatio(InSplitRatio);
	if (SplitRatioVertical == ClampedSplitRatio)
	{
		return;
	}

	SplitRatioVertical = ClampedSplitRatio;
	OnResize();
	RequestEditorLayoutSave();
}

void SSplitterC::SetSplitRatioHorizontal(float InSplitRatio)
{
	const float ClampedSplitRatio = ClampHorizontalRatio(InSplitRatio);
	if (SplitRatioHorizontal == ClampedSplitRatio)
	{
		return;
	}

	SplitRatioHorizontal = ClampedSplitRatio;
	OnResize();
	RequestEditorLayoutSave();
}

void SSplitterC::Tick(float DeltaTime)
{
	if (SideLT)
	{
		SideLT->Tick(DeltaTime);
	}
	if (SideLB)
	{
		SideLB->Tick(DeltaTime);
	}
	if (SideRT)
	{
		SideRT->Tick(DeltaTime);
	}
	if (SideRB)
	{
		SideRB->Tick(DeltaTime);
	}
}

void SSplitterC::Render()
{
	if (SideLT)
	{
		SideLT->Render();
	}
	if (SideLB)
	{
		SideLB->Render();
	}
	if (SideRT)
	{
		SideRT->Render();
	}
	if (SideRB)
	{
		SideRB->Render();
	}
}

SWindow* SSplitterC::GetWindow(FPoint coord)
{
	if (SideLT && SideLT->ISHover(coord))
	{
		return SideLT->GetWindow(coord);
	}
	if (SideLB && SideLB->ISHover(coord))
	{
		return SideLB->GetWindow(coord);
	}
	if (SideRT && SideRT->ISHover(coord))
	{
		return SideRT->GetWindow(coord);
	}
	if (SideRB && SideRB->ISHover(coord))
	{
		return SideRB->GetWindow(coord);
	}

	return nullptr;
}

bool SSplitterC::HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (bDragging && IsMouseMessage(Msg))
	{
		return true;
	}

	auto DispatchToChild = [&](SWindow* Child) -> bool
	{
		return Child && Child->HandleMessage(Core, Hwnd, Msg, WParam, LParam);
	};

	if (IsMouseMessage(Msg))
	{
		const FPoint MousePoint(static_cast<float>(GET_X_LPARAM(LParam)), static_cast<float>(GET_Y_LPARAM(LParam)));
		if (SideLT && SideLT->ISHover(MousePoint))
		{
			return DispatchToChild(SideLT);
		}
		if (SideLB && SideLB->ISHover(MousePoint))
		{
			return DispatchToChild(SideLB);
		}
		if (SideRT && SideRT->ISHover(MousePoint))
		{
			return DispatchToChild(SideRT);
		}
		if (SideRB && SideRB->ISHover(MousePoint))
		{
			return DispatchToChild(SideRB);
		}

		return false;
	}

	return
		DispatchToChild(SideLT) ||
		DispatchToChild(SideLB) ||
		DispatchToChild(SideRT) ||
		DispatchToChild(SideRB);
}

FRect SSplitterC::GetVerticalSplitterRect() const
{
	const float LeftWidth = GetAvailableWidth() * SplitRatioHorizontal;
	return FRect(
		{ Rect.Position.X + LeftWidth, Rect.Position.Y },
		{ SplitterThickness, Rect.Size.Y });
}

FRect SSplitterC::GetHorizontalSplitterRect() const
{
	const float TopHeight = GetAvailableHeight() * SplitRatioVertical;
	return FRect(
		{ Rect.Position.X, Rect.Position.Y + TopHeight },
		{ Rect.Size.X, SplitterThickness });
}

void SSplitterC::DrawSplitterHandles()
{
	struct FHandleDesc
	{
		const char* NameSuffix;
		FRect Rect;
		ImGuiMouseCursor Cursor;
		EDragHandle Handle;
	};

	const FHandleDesc Handles[] =
	{
		{ "Vertical", GetVerticalSplitterRect(), ImGuiMouseCursor_ResizeEW, EDragHandle::Vertical },
		{ "Horizontal", GetHorizontalSplitterRect(), ImGuiMouseCursor_ResizeNS, EDragHandle::Horizontal },
	};

	for (const FHandleDesc& HandleDesc : Handles)
	{
		if (HandleDesc.Rect.Size.X <= 0.0f || HandleDesc.Rect.Size.Y <= 0.0f)
		{
			continue;
		}

		char OverlayName[64];
		std::snprintf(OverlayName, sizeof(OverlayName), "##QuadSplitterOverlay_%s_%p", HandleDesc.NameSuffix, static_cast<void*>(this));

		ImGui::SetNextWindowPos(ToScreenPosition(HandleDesc.Rect.Position), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(HandleDesc.Rect.Size.X, HandleDesc.Rect.Size.Y), ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		if (ImGui::Begin(OverlayName, nullptr, GetSplitterOverlayWindowFlags()))
		{
			ImGui::PushID(this);
			ImGui::PushID(static_cast<int>(HandleDesc.Handle));
			ImGui::InvisibleButton("##Handle", ImVec2(HandleDesc.Rect.Size.X, HandleDesc.Rect.Size.Y));

			const bool bHovered = ImGui::IsItemHovered();
			const bool bActive = ImGui::IsItemActive();
			if (bActive)
			{
				ActiveHandle = HandleDesc.Handle;
				bDragging = true;
			}
			else if (ActiveHandle == HandleDesc.Handle)
			{
				ActiveHandle = EDragHandle::None;
			}

			if (bHovered || bActive)
			{
				ImGui::SetMouseCursor(HandleDesc.Cursor);
			}

			if (bActive)
			{
				if (HandleDesc.Handle == EDragHandle::Vertical)
				{
					const float AvailableWidth = GetAvailableWidth();
					if (AvailableWidth > 0.0f)
					{
						SetSplitRatioHorizontal(SplitRatioHorizontal + (ImGui::GetIO().MouseDelta.x / AvailableWidth));
					}
				}
				else if (HandleDesc.Handle == EDragHandle::Horizontal)
				{
					const float AvailableHeight = GetAvailableHeight();
					if (AvailableHeight > 0.0f)
					{
						SetSplitRatioVertical(SplitRatioVertical + (ImGui::GetIO().MouseDelta.y / AvailableHeight));
					}
				}
			}

			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			DrawList->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), GetSplitterColor(bHovered, bActive));

			ImGui::PopID();
			ImGui::PopID();
		}

		ImGui::End();
		ImGui::PopStyleVar(2);
	}

	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		bDragging = false;
		ActiveHandle = EDragHandle::None;
	}
}

void SSplitterC::Draw()
{
	OnResize();

	if (SideLT)
	{
		SideLT->Draw();
	}
	if (SideLB)
	{
		SideLB->Draw();
	}
	if (SideRT)
	{
		SideRT->Draw();
	}
	if (SideRB)
	{
		SideRB->Draw();
	}

	DrawSplitterHandles();
}
