#include "CubeComponent.h"
#include "Primitive/PrimitiveCube.h"

UCubeComponent::UCubeComponent()
{
	Primitive = std::make_unique<CPrimitiveCube>();
}
