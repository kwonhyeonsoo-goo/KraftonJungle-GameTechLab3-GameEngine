#pragma once
#include "CoreMinimal.h"
#include "Types/CoreTypes.h"

struct FRect {
	float TopLeftX;
	float TopLeftY;
	float Height;
	float Width;
};

struct FPoint {
	float PointX;
	float PointY;

};

class FViewport;

class SWindow
{

protected:
	virtual void UpdateInput(uint32 Msg, uint32 Wparam, uint64 Lparam );
	int WindowId;
	std::unique_ptr<FViewport> Viewport; // 리프 노드일 때만 사용

	FRect Rect;
public :
	SWindow();
	~SWindow();

	virtual void Initialize(FRect rect);

	static int NextWindowId;


	FViewport* GetViewport() { return Viewport.get(); }
	void SetViewport(std::unique_ptr<FViewport> viewport) { Viewport = std::move(viewport); }
	virtual bool IsHover(FPoint coord) const;

	virtual void UpdateNewSize(FRect newRect);
	FRect& GetWindowSize() { return Rect; }
};

