#include "SkyComponent.h"
#include "Primitive/PrimitiveSky.h"

namespace
{
	UObject* CreateUSkyComponentInstance(UObject* InOuter, const FString& InName)
	{
		return new USkyComponent(USkyComponent::StaticClass(), InName, InOuter);
	}
}

UClass* USkyComponent::StaticClass()
{
	static UClass ClassInfo("USkyComponent", UPrimitiveComponent::StaticClass(), &CreateUSkyComponentInstance);
	return &ClassInfo;
}

USkyComponent::USkyComponent()
	: UPrimitiveComponent(StaticClass(), "")
{
	Primitive = std::make_unique<CPrimitiveSky>();
	LocalBoundRadius = 9999.0f; // 항상 Frustum 안에 있도록
}

USkyComponent::USkyComponent(UClass* InClass, const FString& InName, UObject* InOuter)
	: UPrimitiveComponent(InClass, InName, InOuter)
{
	Primitive = std::make_unique<CPrimitiveSky>();
	LocalBoundRadius = 9999.0f;
}
