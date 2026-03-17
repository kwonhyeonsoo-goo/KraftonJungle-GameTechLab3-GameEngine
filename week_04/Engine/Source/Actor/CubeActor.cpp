#include "CubeActor.h"
#include "Component/CubeComponent.h"
#include "Component/RandomColorComponent.h"

namespace
{
	UObject* CreateACubeActorInstance(UObject* InOuter, const FString& InName)
	{
		return new ACubeActor(ACubeActor::StaticClass(), InName, InOuter);
	}
}

UClass* ACubeActor::StaticClass()
{
	static UClass ClassInfo("ACubeActor", AActor::StaticClass(), &CreateACubeActorInstance);
	return &ClassInfo;
}

ACubeActor::ACubeActor(UClass* InClass, const FString& InName, UObject* InOuter)
	: AActor(InClass, InName, InOuter)
{
}

void ACubeActor::PostSpawnInitialize()
{
	PrimitiveComponent = new UCubeComponent();
	AddOwnedComponent(PrimitiveComponent);

	if (bUseRandomColor)
	{
		RandomColorComponent = new URandomColorComponent();
		AddOwnedComponent(RandomColorComponent);
	}

	AActor::PostSpawnInitialize();
}
