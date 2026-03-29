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

	virtual bool isMouseHoverOnBar(FPoint coord) = 0;

public:
	SSplitter();
	~SSplitter();
	static float BarWidth;

};

//split bar => -------
class SSplitterH :public SSplitter	
{

	void UpdateNewSize(FRect newRect) override;
	virtual bool isMouseHoverOnBar(FPoint coord) override ;

public:
	~SSplitterH();

	void Initialize(FRect rect) override;
};

//split bar => |
class SSplitterV : public SSplitter
{
	void UpdateNewSize(FRect newRect) override;
	virtual bool isMouseHoverOnBar(FPoint coord) override;

public:
	SSplitterV();
	~SSplitterV();

	void Initialize(FRect rect) override;

};