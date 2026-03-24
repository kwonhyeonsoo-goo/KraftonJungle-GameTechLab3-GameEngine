#pragma once
#include "PrimitiveComponent.h"

class ENGINE_API UTextComponent : public UPrimitiveComponent
{
public:
	DECLARE_RTTI(UTextComponent, UPrimitiveComponent)

	void Initialize();

	virtual FBoxSphereBounds GetWorldBounds() const override;

	void SetText(const FString& InText) { Text = InText; }
	const FString& GetText() const { return Text; }

	void SetTextColor(const FVector4& InColor) { TextColor = InColor; }
	const FVector4& GetTextColor() const { return TextColor; }

	void SetBillboard(bool bInBillboard) { bBillboard = bInBillboard; }
	bool IsBillboard() const { return bBillboard; }

	void SetBillboardScale(float InScale) { BillboardScale = InScale; }
	float GetBillboardScale() const { return BillboardScale; }

private:
	FString Text = "Text";
	FVector4 TextColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	bool bBillboard = false;
	float BillboardScale = 0.3f;
};