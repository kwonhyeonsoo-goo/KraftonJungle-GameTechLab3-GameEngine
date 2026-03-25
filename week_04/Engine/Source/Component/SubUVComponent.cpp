#include "SubUVComponent.h"
#include "Object/Class.h"
#include "Primitive/PrimitiveBase.h"

IMPLEMENT_RTTI(USubUVComponent, UPrimitiveComponent)

void USubUVComponent::Initialize()
{
	// SubUV 렌더링용 메시 객체 생성
	SubUVMesh = std::make_shared<FMeshData>();
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