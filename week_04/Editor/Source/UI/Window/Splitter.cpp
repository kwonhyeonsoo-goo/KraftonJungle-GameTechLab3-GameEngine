#include "Splitter.h"
#include "Debug/EngineLog.h"

float SSplitter::BarWidth = 5.f;


SSplitter::SSplitter()
{
}

SSplitter::~SSplitter()
{
	delete SideLT;
	delete SideLT;
}


//split bar => -------

void SSplitterH::UpdateNewSize(FRect newRect)
{
	SWindow::UpdateNewSize(newRect);

	Bar.Height = SSplitter::BarWidth;
	Bar.Width = newRect.Width;
	Bar.TopLeftX = newRect.TopLeftX;
	Bar.TopLeftY = newRect.TopLeftY - newRect.Height/2;

	if (SideLT) {
		FRect rectLT = SideLT->GetWindowSize();
		rectLT.TopLeftX = newRect.TopLeftX; rectLT.TopLeftY = newRect.TopLeftY;
		rectLT.Width = newRect.Width;
		rectLT.Height = newRect.Height/2;
		SideLT->UpdateNewSize(rectLT);

	}
	if (SideRB) {
		FRect rectRB = SideLT->GetWindowSize();
		rectRB.TopLeftX = newRect.TopLeftX;
		rectRB.TopLeftY = newRect.TopLeftY + (newRect.Height / 2);
		rectRB.Width = newRect.Width;
		rectRB.Height = newRect.Height / 2;
		SideLT->UpdateNewSize(rectRB);
	}
}

bool SSplitterH::isMouseHoverOnBar(FPoint coord)
{
	float minX = Bar.TopLeftX;
	float maxX = Bar.TopLeftX + Bar.Width;
	float minY = Bar.TopLeftY;
	float maxY = Bar.TopLeftY + Bar.Height;

	if (coord.PointX >= minX && coord.PointY <= maxX
		&& coord.PointY >= minY && coord.PointY <= maxY)
	{
		UE_LOG("Mouse is Hovering on Bar from Window ID : %d", WindowId);
		return true;
	}

	return false;
}


SSplitterH::~SSplitterH()
{
}

void SSplitterH::Initialize(FRect rect)
{
	Rect = rect;

	Bar.Height = SSplitter::BarWidth;
	Bar.Width = Rect.Width;
	Bar.TopLeftX = Rect.TopLeftX;
	Bar.TopLeftY = Rect.Height - Bar.Height / 2;

	FRect RectLT = {};
	RectLT.Height = rect.Height / 2;
	RectLT.Width = rect.Width;
	RectLT.TopLeftX = rect.TopLeftX;
	RectLT.TopLeftY = rect.TopLeftY;
	SideLT = new SWindow();
	SideLT->Initialize(RectLT);

	FRect RectRB = {};
	RectRB.Height = rect.Height / 2;
	RectRB.Width = rect.Width;
	RectRB.TopLeftX = rect.TopLeftX + RectRB.Height;
	RectRB.TopLeftY = rect.TopLeftY;
	SideRB = new SWindow();
	SideRB->Initialize(RectRB);
}

//split bar => |

void SSplitterV::UpdateNewSize(FRect newRect)
{
	SWindow::UpdateNewSize(newRect);

	Bar.Height = newRect.Height;
	Bar.Width = SSplitter::BarWidth;
	Bar.TopLeftX = newRect.TopLeftX - Bar.Width / 2;
	Bar.TopLeftY = newRect.TopLeftY;

	UE_LOG("New Bar Size: H%d W%d X%d Y%d", Bar.Height, Bar.Width, Bar.TopLeftX, Bar.TopLeftY);

	if (SideLT) {
		FRect rectLT = SideLT->GetWindowSize();
		rectLT.TopLeftX = newRect.TopLeftX; rectLT.TopLeftY = newRect.TopLeftY;
		rectLT.Width = newRect.Width / 2;
		rectLT.Height = newRect.Height;
		SideLT->UpdateNewSize(rectLT);

	}
	if (SideRB) {
		FRect rectRB = SideLT->GetWindowSize();
		rectRB.TopLeftX = newRect.TopLeftX + newRect.Width / 2;
		rectRB.TopLeftY = newRect.TopLeftY;
		rectRB.Width = newRect.Width /2;
		rectRB.Height = newRect.Height;
		SideLT->UpdateNewSize(rectRB);
	}
}

bool SSplitterV::isMouseHoverOnBar(FPoint coord)
{
	return false;
}

SSplitterV::SSplitterV()
{
	WindowId = SWindow::NextWindowId++;
}

SSplitterV::~SSplitterV()
{
}

void SSplitterV::Initialize(FRect rect)
{
}

