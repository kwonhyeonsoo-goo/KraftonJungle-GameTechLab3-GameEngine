#include "CubeComponent.h"
#include "Primitive/PrimitiveCube.h"

namespace
{
	UObject* CreateUCubeComponentInstance(UObject* InOuter, const FString& InName)
	{
		return new UCubeComponent(UCubeComponent::StaticClass(), InName, InOuter);
	}
}

UClass* UCubeComponent::StaticClass()
{
	static UClass ClassInfo("UCubeComponent", UPrimitiveComponent::StaticClass(), &CreateUCubeComponentInstance);
	return &ClassInfo;
}

UCubeComponent::UCubeComponent()
	: UPrimitiveComponent(StaticClass(), "")
{
	Primitive = std::make_unique<CPrimitiveCube>();
	LocalBoundRadius = 0.866f; // sqrt(3) * 0.5
}

UCubeComponent::UCubeComponent(UClass* InClass, const FString& InName, UObject* InOuter)
	: UPrimitiveComponent(InClass, InName, InOuter)
{
	Primitive = std::make_unique<CPrimitiveCube>();
	LocalBoundRadius = 0.866f; // sqrt(3) * 0.5
}
