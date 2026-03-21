#include "PrimitiveComponent.h"

IMPLEMENT_RTTI(UPrimitiveComponent, USceneComponent)

// TODO: Ritter's Algorithm으로 개선
void UPrimitiveComponent::UpdateLocalBoundRadius(FVector InNewPointLocalPos)
{
	if (InNewPointLocalPos.SizeSquared() > LocalBoundRadius * LocalBoundRadius)
		LocalBoundRadius = InNewPointLocalPos.Size() + 0.01f;	// 약간의 여유 추가
}
