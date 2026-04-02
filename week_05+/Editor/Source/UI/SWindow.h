#pragma once
#include "Math/Rect.h"
#include "Windows.h"
#include "imgui.h"

class FCore;

enum SplitDirection
{
	Horizontal,
	Vertical,
	Cross,
};

enum SplitOption
{
	LT,
	RT, 
	LB,
	RB,	
};

class SSplitter;
class SSplitterC;

class SWindow
{
	SWindow* Parent = nullptr;

protected:
	FRect Rect;

public:
	SWindow() = default;
	SWindow(FRect InRect) : Rect(InRect) {}
	virtual ~SWindow() = default;

	void SetParent(SWindow* InParent) { Parent = InParent; }
	SWindow* GetParent() const { return Parent; }
	void SetRect(const FRect& InRect);
	virtual bool ISHover(FPoint coord) const;
	virtual SWindow* GetWindow(FPoint coord);
	FRect GetRect() const { return Rect; }
	virtual SWindow* Merge(SWindow* MainWindow); // 부모를 제거하고 메인 윈도우로 대체 다른 윈도우는 삭제
	virtual void OnResize() {}
	virtual void Tick(float DeltaTime) {}
	virtual void Render() {}
	virtual void Draw() {}
	virtual bool HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam) { return false; }
	virtual void ReplaceSide(SWindow* OldSide, SWindow* NewSide);
	SSplitter* Split(SWindow* InNewWindow, SplitDirection InDirection, SplitOption InSplitOption);
	SSplitterC* Split4(SWindow* InNewWindow1, SWindow* InNewWindow2, SWindow* InNewWindow3, SplitOption InSplitOption);
};

class SSplitter : public SWindow
{
	virtual SWindow* GetWindow(FPoint coord) override;

protected:
	float SplitRatio = 0.5f;
	float MinPaneSize = 80.0f;
	float SplitterThickness = 6.0f;
	bool bDragging = false;
	SWindow* SideLT = nullptr;
	SWindow* SideRB = nullptr;

	float ClampSplitRatio(float InSplitRatio) const;
	float GetAvailablePrimaryAxisSize() const;
	void InitializeSplitterLayout();
	void DrawSplitterHandle();
	virtual float GetPrimaryAxisSize() const = 0;
	virtual float GetMouseDeltaForSplit() const = 0;
	virtual ImGuiMouseCursor GetSplitterMouseCursor() const = 0;
	virtual FRect GetSplitterRect() const = 0;

public:
	SWindow* GetSideLT() { return SideLT; }
	const SWindow* GetSideLT() const { return SideLT; }
	SWindow* GetSideRB() { return SideRB; }
	const SWindow* GetSideRB() const { return SideRB; }
	virtual void SetSideLT(SWindow* InSideLT);
	virtual void SetSideRB(SWindow* InSideRB);
	virtual FRect GetSideLTRect() = 0;
	virtual FRect GetSideRBRect() = 0;
	virtual void OnResize() override;
	void ReplaceSide(SWindow* OldSide, SWindow* NewSide) override;
	SSplitter(FRect InRect, SWindow* InSideLT = nullptr, SWindow* InSideRB = nullptr, float InSplitRatio = 0.5f);
	~SSplitter() override;
	virtual SWindow* Merge(SWindow* MainWindow) override; // 부모를 제거하고 메인 윈도우로 대체 다른 윈도우는 삭제
	float GetSplitRatio() const { return SplitRatio; }
	virtual void SetSplitRatio(float InSplitRatio);
	void SetMinPaneSize(float InMinPaneSize) { MinPaneSize = InMinPaneSize; OnResize(); }
	float GetMinPaneSize() const { return MinPaneSize; }
	void SetSplitterThickness(float InSplitterThickness) { SplitterThickness = InSplitterThickness; OnResize(); }
	float GetSplitterThickness() const { return SplitterThickness; }
	virtual void Tick(float DeltaTime) override;
	virtual void Render() override;
	virtual bool HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam) override;
	virtual void Draw() override;

};

class SSplitterV : public SSplitter
{
public:
	SSplitterV(FRect InRect, SWindow* InSideLT = nullptr, SWindow* InSideRB = nullptr, float InSplitRatio = 0.5f)
		: SSplitter(InRect, InSideLT, InSideRB, InSplitRatio)
	{
		InitializeSplitterLayout();
	}

	virtual FRect GetSideLTRect() override;
	virtual FRect GetSideRBRect() override;
	virtual float GetPrimaryAxisSize() const override;
	virtual float GetMouseDeltaForSplit() const override;
	virtual ImGuiMouseCursor GetSplitterMouseCursor() const override;
	virtual FRect GetSplitterRect() const override;
	virtual void Draw() override;
};

class SSplitterH : public SSplitter
{
public:
	SSplitterH(FRect InRect, SWindow* InSideLT = nullptr, SWindow* InSideRB = nullptr, float InSplitRatio = 0.5f)
		: SSplitter(InRect, InSideLT, InSideRB, InSplitRatio)
	{
		InitializeSplitterLayout();
	}

	virtual FRect GetSideLTRect() override;
	virtual FRect GetSideRBRect() override;
	virtual float GetPrimaryAxisSize() const override;
	virtual float GetMouseDeltaForSplit() const override;
	virtual ImGuiMouseCursor GetSplitterMouseCursor() const override;
	virtual FRect GetSplitterRect() const override;
	virtual void Draw() override;
};

class SSplitterC : public SWindow
{
	virtual SWindow* GetWindow(FPoint coord) override;

	SWindow* SideLT = nullptr;
	SWindow* SideLB = nullptr;
	SWindow* SideRT = nullptr;
	SWindow* SideRB = nullptr;
	float MinPaneSize = 80.0f;
	float SplitterThickness = 6.0f;
	bool bDragging = false;
	enum class EDragHandle : unsigned char
	{
		None,
		Vertical,
		Horizontal,
	};
	EDragHandle ActiveHandle = EDragHandle::None;

	float SplitRatioVertical = 0.5f;
	float SplitRatioHorizontal = 0.5f;

	float ClampHorizontalRatio(float InSplitRatio) const;
	float ClampVerticalRatio(float InSplitRatio) const;
	float GetAvailableWidth() const;
	float GetAvailableHeight() const;
	FRect GetVerticalSplitterRect() const;
	FRect GetHorizontalSplitterRect() const;
	void DrawSplitterHandles();

public:
	SWindow* GetSideLT() { return SideLT; }
	const SWindow* GetSideLT() const { return SideLT; }
	SWindow* GetSideLB() { return SideLB; }
	const SWindow* GetSideLB() const { return SideLB; }
	SWindow* GetSideRT() { return SideRT; }
	const SWindow* GetSideRT() const { return SideRT; }
	SWindow* GetSideRB() { return SideRB; }
	const SWindow* GetSideRB() const { return SideRB; }
	virtual void SetSideLT(SWindow* InSideLT);
	virtual void SetSideLB(SWindow* InSideLB);
	virtual void SetSideRT(SWindow* InSideRT);
	virtual void SetSideRB(SWindow* InSideRB);
	virtual FRect GetSideLTRect();
	virtual FRect GetSideLBRect();
	virtual FRect GetSideRTRect();
	virtual FRect GetSideRBRect();
	virtual void OnResize() override;
	void ReplaceSide(SWindow* OldSide, SWindow* NewSide) override;
	SSplitterC(FRect InRect, SWindow* InSideLT = nullptr, SWindow* InSideLB = nullptr, SWindow* InSideRT = nullptr, SWindow* InSideRB = nullptr, float InSplitRatioHorizontal = 0.5f, float InSplitRatioVertical = 0.5f);
	~SSplitterC() override;
	virtual SWindow* Merge(SWindow* MainWindow) override; // 부모를 제거하고 메인 윈도우로 대체 다른 윈도우는 삭제

	float GetSplitRatioVertical() const { return SplitRatioVertical; }
	float GetSplitRatioHorizontal() const { return SplitRatioHorizontal; }
	virtual void SetSplitRatioVertical(float InSplitRatio);
	virtual void SetSplitRatioHorizontal(float InSplitRatio);
	void SetMinPaneSize(float InMinPaneSize) { MinPaneSize = InMinPaneSize; OnResize(); }
	float GetMinPaneSize() const { return MinPaneSize; }
	void SetSplitterThickness(float InSplitterThickness) { SplitterThickness = InSplitterThickness; OnResize(); }
	float GetSplitterThickness() const { return SplitterThickness; }
	virtual void Tick(float DeltaTime) override;
	virtual void Render() override;
	virtual bool HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam) override;
	virtual void Draw() override;
};
