#include "GizmoComponent.h"
#include "Object/Class.h"
#include "Primitive/PrimitiveGizmo.h"

namespace
{
	UObject* CreateUGizmoComponentInstance(UObject* InOuter, const FString& InName)
	{
		return new UGizmoComponent();
	}
}


UClass* UGizmoComponent::StaticClass()
{
	static UClass ClassInfo("UGizmoComponent", UGizmoComponent::StaticClass(), &CreateUGizmoComponentInstance);
	return &ClassInfo;
}

UGizmoComponent::UGizmoComponent()
	: UPrimitiveComponent(StaticClass(), "")
{
	Primitive = std::make_unique<CPrimitiveGizmo>();
	LocalBoundRadius = 1.0f;
}
