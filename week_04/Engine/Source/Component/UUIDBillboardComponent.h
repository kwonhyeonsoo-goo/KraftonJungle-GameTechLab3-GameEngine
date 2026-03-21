#pragma once

#include "PrimitiveComponent.h"

class ENGINE_API UUUIDBillboardComponent : public UPrimitiveComponent
{
public:
	DECLARE_RTTI(UUUIDBillboardComponent, UPrimitiveComponent)

	void Initialize();

	FString GetDisplayText() const;
	FVector GetTextWorldPosition() const;

	float GetWorldScale() const { return BillboardScale; }
	const FVector4& GetTextColor() const { return TextColor; }

	void SetWorldOffset(const FVector& InOffset) { WorldOffset = InOffset; }
	const FVector& GetWorldOffset() const { return WorldOffset; }

	void SetWorldScale(float InScale) { BillboardScale = InScale; }
	void SetTextColor(const FVector4& InColor) { TextColor = InColor; }

	FBoundingSphere GetWorldBounds() const override;
	FBoxSphereBounds GetWorldBoundsForAABB() const override;

private:
	FVector WorldOffset = FVector(0.0f, 0.0f, 0.3f);

	float BillboardScale = 0.3f;

	FVector4 TextColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
};