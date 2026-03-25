#include "SkyComponent.h"
#include "Primitive/PrimitiveSky.h"
#include "Object/Class.h"
IMPLEMENT_RTTI(USkyComponent, UPrimitiveComponent)

void USkyComponent::Initialize()
{
	Primitive = std::make_unique<CPrimitiveSky>();
}