#pragma once
#include "Window.h"


class SSplitter :
    public SWindow
{

	bool bIsSelected;

protected:
	FRect Bar;	//바 위치만 이동, 사이즈 BarWidth 는 고정
	SWindow* SideLT; // Left or Top
	SWindow* SideRB; // Right or Bottom
	float SplitRatio = 0.5f;  // 

public:
	SSplitter();
	~SSplitter();
	static float BarWidth;
	virtual SSplitter* isMouseHoverOnBar(FPoint coord) = 0;
	virtual void UpdateBarPosition(FPoint detlaCoord) = 0;

	void SetSideLT(SWindow* sideLT) { SideLT = sideLT; }
	void SetSideRB(SWindow* sideRB) { SideRB = sideRB; }

};

//split bar => -------
class SSplitterH :public SSplitter	
{

	void UpdateNewSize(FRect newRect) override;
	virtual SSplitter* isMouseHoverOnBar(FPoint coord) override ;

public:
	~SSplitterH();

	void Initialize(FRect rect) override;

	void UpdateBarPosition(FPoint detlaCoord) override;
};

//split bar => |
class SSplitterV : public SSplitter
{
	void UpdateNewSize(FRect newRect) override;
	virtual SSplitter* isMouseHoverOnBar(FPoint coord) override;

public:
	SSplitterV();
	~SSplitterV();

	void Initialize(FRect rect) override;
	void UpdateBarPosition(FPoint detlaCoord) override;


};