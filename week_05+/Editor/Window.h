#pragma once

struct FPoint
{
	float X, Y;
	FPoint(float X, float Y);
};

struct FRect
{
	FPoint MinP, MaxP;
	FRect(FPoint P1, FPoint P2);
};

class SWindow
{
	FRect Rect;
	bool IsHover(FPoint coord) const;
};

class SSplitter : public SWindow
{
	SWindow* SideLT; // Left or Top
	SWindow* SideRB; // Right or Bottom

	~SSplitter();
};

class SSplitterH : public SSplitter
{
};

class SSplitterV : public SSplitter
{
};
