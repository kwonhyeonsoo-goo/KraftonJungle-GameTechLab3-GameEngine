#include "SphereComponent.h"
#include "Primitive/PrimitiveSphere.h"

namespace
{
	UObject* CreateUSphereComponentInstance(UObject* InOuter, const FString& InName)
	{
		return new USphereComponent(USphereComponent::StaticClass(), InName, InOuter);
	}
}

UClass* USphereComponent::StaticClass()
{
	static UClass ClassInfo("USphereComponent", UPrimitiveComponent::StaticClass(), &CreateUSphereComponentInstance);
	return &ClassInfo;
}

USphereComponent::USphereComponent()
	: UPrimitiveComponent(StaticClass(), "")
{
	Primitive = std::make_unique<CPrimitiveSphere>();
	LocalBoundRadius = 1.0f;
}

USphereComponent::USphereComponent(UClass* InClass, const FString& InName, UObject* InOuter)
	: UPrimitiveComponent(InClass, InName, InOuter)
{
	Primitive = std::make_unique<CPrimitiveSphere>();
	LocalBoundRadius = 1.0f;
}
