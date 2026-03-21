#include "GizmoComponent.h"
#include "Primitive/PrimitiveGizmo.h"

IMPLEMENT_RTTI(UGizmoComponent, UPrimitiveComponent)

void UGizmoComponent::Initialize()
{
	Primitive = std::make_unique<CPrimitiveGizmo>();
	LocalBoundRadius = 1.0f;
}