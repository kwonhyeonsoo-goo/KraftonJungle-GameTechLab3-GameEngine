#include "Splitter.h"
#include "Debug/EngineLog.h"

#include <algorithm>
#include "UI/Viewport.h"
float SSplitter::BarWidth = 5.f;


SSplitter::SSplitter()
{
}

SSplitter::~SSplitter()
{
	delete SideLT;
	delete SideRB;
}

void SSplitter::AddIfMouseHoverOnBar(FPoint coord, TArray<SSplitter*>& outArray)
{
	if (isMouseHoverOnBar(coord)) {
		outArray.push_back(this);
	}
	if (SSplitter* splitterSideLT = dynamic_cast<SSplitter*>(SideLT))
	{
		splitterSideLT->AddIfMouseHoverOnBar(coord, outArray);

	}

	if (SSplitter* splitterSideRB = dynamic_cast<SSplitter*>(SideRB))
	{
		splitterSideRB->AddIfMouseHoverOnBar(coord, outArray);

	}

}

bool SSplitter::IsAnyBarHovered(FPoint pt)
{
	if (isMouseHoverOnBar(pt)) return true;
	if (SSplitter* it = dynamic_cast<SSplitter*>(GetSideLT())) {
		if (it->IsAnyBarHovered(pt)) { return true; }
	}

	if (SSplitter* it = dynamic_cast<SSplitter*>(GetSideRB())) {
		if (it->IsAnyBarHovered(pt)) { return true; }
	}
	return false;
}


//split bar => -------


SSplitterH::~SSplitterH()
{
}

void SSplitterH::Initialize(FRect rect)
{
	Rect = rect;

	Bar.Height = SSplitter::BarWidth;
	Bar.Width = Rect.Width;
	Bar.TopLeftX = Rect.TopLeftX;
	Bar.TopLeftY = Rect.TopLeftY + Rect.Height / 2 - Bar.Height / 2;

	FRect RectLT = {};
	RectLT.Height = rect.Height / 2;
	RectLT.Width = rect.Width;
	RectLT.TopLeftX = rect.TopLeftX;
	RectLT.TopLeftY = rect.TopLeftY;
	if (SideLT) SideLT->Initialize(RectLT);

	FRect RectRB = {};
	RectRB.Height = rect.Height / 2;
	RectRB.Width = rect.Width;
	RectRB.TopLeftX = rect.TopLeftX;
	RectRB.TopLeftY = rect.TopLeftY + rect.Height / 2;

	if (SideRB) SideRB->Initialize(RectRB);
}

void SSplitterH::UpdateBarPosition(FPoint detlaCoord)
{
	float totalHeight = SideLT->GetWindowSize().Height + SideRB->GetWindowSize().Height;
	if (totalHeight > 0.f)
		SplitRatio += detlaCoord.PointY / totalHeight;
	SplitRatio = max(0.1f, min(0.9f, SplitRatio));  // 범위 제한

	// Bar, SideLT, SideRB는 UpdateNewSize가 처리하게 위임
	UpdateNewSize(Rect);

	UE_LOG("New Bar Position : ( %f )", Bar.TopLeftY);
}

void SSplitterH::SetRatio(float newRatio)
{
	SplitRatio = newRatio;
	UpdateNewSize(Rect);
}

//전체 크기가 바뀌었을때
void SSplitterH::UpdateNewSize(FRect newRect)
{
	SWindow::UpdateNewSize(newRect);

	float splitHeight = newRect.Height * SplitRatio;
	splitHeight = max(1.f, min(newRect.Height - 1.f, splitHeight));

	float halfBar = Bar.Height / 2;

	Bar.Height = SSplitter::BarWidth;
	Bar.Width = newRect.Width;
	Bar.TopLeftX = newRect.TopLeftX;
	Bar.TopLeftY = newRect.TopLeftY + splitHeight - halfBar;

	if (SideLT) {
		FRect rectLT;
		rectLT.TopLeftX = newRect.TopLeftX;
		rectLT.TopLeftY = newRect.TopLeftY;
		rectLT.Width = newRect.Width;
		rectLT.Height = max(1.f, splitHeight - halfBar);  // 바 절반만큼 빼기
		SideLT->UpdateNewSize(rectLT);
	}
	if (SideRB) {
		FRect rectRB;
		rectRB.TopLeftX = newRect.TopLeftX;
		rectRB.TopLeftY = newRect.TopLeftY + splitHeight + halfBar;  // 바 절반만큼 밀기
		rectRB.Width = newRect.Width;
		rectRB.Height = max(1.f, newRect.Height - splitHeight - halfBar);
		SideRB->UpdateNewSize(rectRB);
	}
}

bool SSplitterH::isMouseHoverOnBar(FPoint coord)
{
	float minX = Bar.TopLeftX;
	float maxX = Bar.TopLeftX + Bar.Width;
	float minY = Bar.TopLeftY;
	float maxY = Bar.TopLeftY + Bar.Height;
	// isMouseHoverOnBar 안에서
	UE_LOG("Bar: %f %f %f %f", Bar.TopLeftX, Bar.TopLeftY, Bar.TopLeftX + Bar.Width, Bar.TopLeftY + Bar.Height);
	if (coord.PointX >= minX && coord.PointX <= maxX
		&& coord.PointY >= minY && coord.PointY <= maxY)
	{
		return true;
	}

	return false;
}



//split bar => |

SSplitterV::SSplitterV()
{
	WindowId = SWindow::NextWindowId++;
}

SSplitterV::~SSplitterV()
{
}

void SSplitterV::Initialize(FRect rect)
{
	Rect = rect;

	Bar.Width = SSplitter::BarWidth;
	Bar.Height = Rect.Height;
	Bar.TopLeftX = Rect.TopLeftX + Rect.Width / 2 - Bar.Width / 2;
	Bar.TopLeftY = Rect.TopLeftY;

	FRect RectLT = {};
	RectLT.Width = rect.Width / 2;
	RectLT.Height = rect.Height;
	RectLT.TopLeftX = rect.TopLeftX;
	RectLT.TopLeftY = rect.TopLeftY;
	if (SideLT) SideLT->Initialize(RectLT);

	FRect RectRB = {};
	RectRB.Width = rect.Width / 2;
	RectRB.Height = rect.Height;
	RectRB.TopLeftX = rect.TopLeftX + rect.Width / 2;
	RectRB.TopLeftY = rect.TopLeftY;
	if (SideRB) SideRB->Initialize(RectRB);
}

void SSplitterV::UpdateBarPosition(FPoint detlaCoord)
{
	Bar.TopLeftX += detlaCoord.PointX;

	float totalWidth = SideLT->GetWindowSize().Width + SideRB->GetWindowSize().Width;
	if (totalWidth > 0.f)
		SplitRatio += detlaCoord.PointX / totalWidth;
	SplitRatio = max(0.1f, min(0.9f, SplitRatio));

	UpdateNewSize(Rect);

	UE_LOG("New Bar Position : ( %f, %f )", detlaCoord.PointX, detlaCoord.PointY);
}

void SSplitterV::SetRatio(float newRatio)
{
	SplitRatio = newRatio;
	UpdateNewSize(Rect);
}

//전체 크기가 바뀌었을때
void SSplitterV::UpdateNewSize(FRect newRect)
{
	SWindow::UpdateNewSize(newRect);

	float splitWidth = newRect.Width * SplitRatio;
	splitWidth = max(1.f, min(newRect.Width - 1.f, splitWidth));

	float halfBar = Bar.Width / 2;  // = BarWidth / 2

	Bar.Width = SSplitter::BarWidth;
	Bar.Height = newRect.Height;
	Bar.TopLeftX = newRect.TopLeftX + splitWidth - halfBar;
	Bar.TopLeftY = newRect.TopLeftY;

	if (SideLT) {
		FRect rectLT;
		rectLT.TopLeftX = newRect.TopLeftX;
		rectLT.TopLeftY = newRect.TopLeftY;
		rectLT.Width = max(1.f, splitWidth - halfBar);   // 바 절반만큼 빼기
		rectLT.Height = newRect.Height;
		SideLT->UpdateNewSize(rectLT);
	}
	if (SideRB) {
		FRect rectRB;
		rectRB.TopLeftX = newRect.TopLeftX + splitWidth + halfBar;  // 바 절반만큼 밀기
		rectRB.TopLeftY = newRect.TopLeftY;
		rectRB.Width = max(1.f, newRect.Width - splitWidth - halfBar);
		rectRB.Height = newRect.Height;
		SideRB->UpdateNewSize(rectRB);
	}
}

bool SSplitterV::isMouseHoverOnBar(FPoint coord)
{
	float minX = Bar.TopLeftX;
	float maxX = Bar.TopLeftX + Bar.Width;
	float minY = Bar.TopLeftY;
	float maxY = Bar.TopLeftY + Bar.Height;

	if (coord.PointX >= minX && coord.PointX <= maxX
		&& coord.PointY >= minY && coord.PointY <= maxY)
	{
		return this;
	}
	UE_LOG("Bar: %f %f %f %f", Bar.TopLeftX, Bar.TopLeftY, Bar.TopLeftX + Bar.Width, Bar.TopLeftY + Bar.Height);
	if (coord.PointX >= minX && coord.PointX <= maxX
		&& coord.PointY >= minY && coord.PointY <= maxY)
	{
		return true;
	}

	return false;

}