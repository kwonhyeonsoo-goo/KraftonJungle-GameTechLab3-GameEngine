#include "SphereComponent.h"
#include "Primitive/PrimitiveSphere.h"

IMPLEMENT_RTTI(USphereComponent, UPrimitiveComponent)

void USphereComponent::Initialize()
{
	Primitive = std::make_unique<CPrimitiveSphere>();
	LocalBoundRadius = 1.0f;
}