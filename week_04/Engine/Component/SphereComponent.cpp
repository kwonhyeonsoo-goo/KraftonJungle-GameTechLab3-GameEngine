#include "SphereComponent.h"
#include "Primitive/PrimitiveSphere.h"

USphereComponent::USphereComponent()
{
	Primitive = std::make_unique<CPrimitiveSphere>();
}
