#include "SubUVComponent.h"
#include "Object/Class.h"

IMPLEMENT_RTTI(USubUVComponent, UPrimitiveComponent)

void USubUVComponent::Initialize()
{
}

FBoxSphereBounds USubUVComponent::GetWorldBounds() const
{
	const FVector Center = GetWorldLocation();

	const float HalfW = Size.X * 0.3f;
	const float HalfH = Size.Y * 0.3f;
	const float MaxHalf = (HalfW > HalfH) ? HalfW : HalfH;
	const float PickingPadding = 0.0f;

	const FVector BoxExtent(HalfW+PickingPadding, HalfH+PickingPadding, MaxHalf+PickingPadding);

	return { Center, BoxExtent.Size(), BoxExtent };
}