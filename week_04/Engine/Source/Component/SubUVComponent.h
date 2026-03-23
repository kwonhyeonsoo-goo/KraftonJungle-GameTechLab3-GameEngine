#pragma once
#include "PrimitiveComponent.h"

class ENGINE_API USubUVComponent : public UPrimitiveComponent
{
public:
	DECLARE_RTTI(USubUVComponent, UPrimitiveComponent)

	void Initialize();

	virtual FBoxSphereBounds GetWorldBounds() const override;

	void SetSize(const FVector2& InSize) { Size = InSize; }
	const FVector2& GetSize() const { return Size; }

	void SetColumns(int32 InColumns) { Columns = InColumns; }
	int32 GetColumns() const { return Columns; }

	void SetRows(int32 InRows) { Rows = InRows; }
	int32 GetRows() const { return Rows; }

	void SetTotalFrames(int32 InTotalFrames) { TotalFrames = InTotalFrames; }
	int32 GetTotalFrames() const { return TotalFrames; }

	void SetFirstFrame(int32 InFirstFrame) { FirstFrame = InFirstFrame; }
	int32 GetFirstFrame() const { return FirstFrame; }

	void SetLastFrame(int32 InLastFrame) { LastFrame = InLastFrame; }
	int32 GetLastFrame() const { return LastFrame; }

	void SetFPS(float InFPS) { FPS = InFPS; }
	float GetFPS() const { return FPS; }

	void SetLoop(bool bInLoop) { bLoop = bInLoop; }
	bool IsLoop() const { return bLoop; }

	void SetBillboard(bool bInBillboard) { bBillboard = bInBillboard; }
	bool IsBillboard() const { return bBillboard; }

private:
	FVector2 Size = FVector2(0.3f, 0.3f);
	FVector4 Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

	int32 Columns = 9;
	int32 Rows = 4;
	int32 TotalFrames = 36;

	int32 FirstFrame = 0;
	int32 LastFrame = 35;

	float FPS = 8.0f;

	bool bLoop = true;
	bool bBillboard = false;
};