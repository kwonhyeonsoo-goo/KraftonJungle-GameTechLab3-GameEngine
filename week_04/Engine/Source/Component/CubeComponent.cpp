#include "CubeComponent.h"
#include "Primitive/PrimitiveCube.h"

namespace
{
	UObject* CreateUCubeComponentInstance(UObject* InOuter, const FString& InName)
	{
		return new UCubeComponent();
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
