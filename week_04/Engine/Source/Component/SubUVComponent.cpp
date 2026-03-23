#include "SubUVComponent.h"
#include "Object/Class.h"
#include <algorithm>

IMPLEMENT_RTTI(USubUVComponent, UPrimitiveComponent)

void USubUVComponent::Initialize()
{
}

FBoxSphereBounds USubUVComponent::GetWorldBounds() const
{
	const FVector Center = GetWorldLocation();

	const float HalfW = Size.X * 0.5f;
	const float HalfH = Size.Y * 0.5f;
	const float MaxHalf = (HalfW > HalfH) ? HalfW : HalfH;
	const float PickingPadding = 1.0f;

	const FVector BoxExtent(HalfW+PickingPadding, HalfH+PickingPadding, MaxHalf+PickingPadding);

	return { Center, BoxExtent.Size(), BoxExtent };
}