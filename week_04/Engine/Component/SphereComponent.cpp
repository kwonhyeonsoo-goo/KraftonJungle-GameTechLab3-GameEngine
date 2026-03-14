#include "SphereComponent.h"
#include "Primitive/PrimitiveSphere.h"
#include "Object/Class.h"

namespace
{
	UObject* CreateUSphereComponentInstance(UObject* InOuter, const FString& InName)
	{
		return new USphereComponent();
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
}
