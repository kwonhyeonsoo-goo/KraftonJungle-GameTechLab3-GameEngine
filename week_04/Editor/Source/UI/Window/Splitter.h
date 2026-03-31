#pragma once
#include "Window.h"


class SSplitter :
    public SWindow
{
private:
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
	
	virtual bool isMouseHoverOnBar(FPoint coord) = 0;	//자기 자신의 바가 호버 되는지 체크
	virtual void AddIfMouseHoverOnBar(FPoint coord, TArray<SSplitter*>& outArray);	//자기 자신+모든 자식의 바가 호버되면 배열에 추가
	bool IsAnyBarHovered(FPoint pt);	//자기 자신+모든 자식의 바 아무나 호버되면 true 반환

	float GetRatio() { return SplitRatio; }
	virtual void SetRatio(float newRatio) =0;
	virtual void UpdateBarPosition(FPoint detlaCoord) = 0;

	void SetSideLT(SWindow* sideLT) { SideLT = sideLT; }
	void SetSideRB(SWindow* sideRB) { SideRB = sideRB; }

	SWindow* GetSideLT() const { return SideLT; }
	SWindow* GetSideRB() const { return SideRB; }

	FRect GetBarRect() const { return Bar; };
};

//split bar => -------
class SSplitterH :public SSplitter	
{

	void UpdateNewSize(FRect newRect) override;
	virtual bool isMouseHoverOnBar(FPoint coord) override ;

public:
	~SSplitterH();

	void Initialize(FRect rect) override;

	void UpdateBarPosition(FPoint detlaCoord) override;

	virtual void SetRatio(float newRatio) override;

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

	void UpdateBarPosition(FPoint detlaCoord) override;

	virtual void SetRatio(float newRatio) override;

};