#include "PrimitiveComponent.h"

namespace
{
	UObject* CreateUPrimitiveComponentInstance(UObject* InOuter, const FString& InName)
	{
		return new UPrimitiveComponent(UPrimitiveComponent::StaticClass(), InName, InOuter);
	}
}

UClass* UPrimitiveComponent::StaticClass()
{
	static UClass ClassInfo("UPrimitiveComponent", USceneComponent::StaticClass(), &CreateUPrimitiveComponentInstance);
	return &ClassInfo;
}
