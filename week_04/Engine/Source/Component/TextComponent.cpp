#include "TextComponent.h"
#include "Object/Class.h"
#include <algorithm>

IMPLEMENT_RTTI(UTextComponent, UPrimitiveComponent)

void UTextComponent::Initialize()
{
}

FBoxSphereBounds UTextComponent::GetWorldBounds() const
{
	const FVector Center = GetWorldLocation();
	const size_t TextLength = std::max<size_t>(Text.size(), 1);

	const float BaseScale = bBillboard ? BillboardScale : std::max(GetWorldTransform().GetScaleVector().X, 0.3f);
	const float HalfWidth = static_cast<float>(TextLength) * BaseScale * 0.35f;
	const float HalfHeight = BaseScale * 0.5f;
	const float HalfDepth = BaseScale * 0.15f;

	const FVector BoxExtent(HalfDepth, HalfWidth, HalfHeight);
	return { Center, BoxExtent.Size(), BoxExtent };
}