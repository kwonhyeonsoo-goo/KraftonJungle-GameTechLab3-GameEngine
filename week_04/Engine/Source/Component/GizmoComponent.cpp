#include "GizmoComponent.h"
#include "Primitive/PrimitiveGizmo.h"
#include "Object/Class.h"

IMPLEMENT_RTTI(UGizmoComponent, UPrimitiveComponent)

void UGizmoComponent::Initialize()
{
	Primitive = std::make_unique<CPrimitiveGizmo>();
}