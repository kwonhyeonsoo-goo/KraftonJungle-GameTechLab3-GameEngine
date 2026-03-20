#include "SkyComponent.h"
#include "Primitive/PrimitiveSky.h"

IMPLEMENT_RTTI(USkyComponent, UPrimitiveComponent)

void USkyComponent::Initialize()
{
	Primitive = std::make_unique<CPrimitiveSky>();
	LocalBoundRadius = 9999.0f;
}
