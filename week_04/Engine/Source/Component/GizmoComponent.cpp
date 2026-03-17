#include "GizmoComponent.h"
#include "Object/Class.h"
#include "Primitive/PrimitiveGizmo.h"

namespace
{
	UObject* CreateUGizmoComponentInstance(UObject* InOuter, const FString& InName)
	{
		return new UGizmoComponent(UGizmoComponent::StaticClass(), InName, InOuter);
	}
}


UClass* UGizmoComponent::StaticClass()
{
	static UClass ClassInfo("UGizmoComponent", UPrimitiveComponent::StaticClass(), &CreateUGizmoComponentInstance);
	return &ClassInfo;
}

UGizmoComponent::UGizmoComponent()
	: UPrimitiveComponent(StaticClass(), "")
{
	Primitive = std::make_unique<CPrimitiveGizmo>();
	LocalBoundRadius = 1.0f;
}

UGizmoComponent::UGizmoComponent(UClass* InClass, const FString& InName, UObject* InOuter)
	: UPrimitiveComponent(InClass, InName, InOuter)
{
	Primitive = std::make_unique<CPrimitiveGizmo>();
	LocalBoundRadius = 1.0f;
}
