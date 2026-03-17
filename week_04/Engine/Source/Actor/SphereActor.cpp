#include "SphereActor.h"
#include "Component/SphereComponent.h"
#include "Component/RandomColorComponent.h"

namespace
{
	UObject* CreateASphereActorInstance(UObject* InOuter, const FString& InName)
	{
		return new ASphereActor(ASphereActor::StaticClass(), InName, InOuter);
	}
}

UClass* ASphereActor::StaticClass()
{
	static UClass ClassInfo("ASphereActor", AActor::StaticClass(), &CreateASphereActorInstance);
	return &ClassInfo;
}

ASphereActor::ASphereActor(UClass* InClass, const FString& InName, UObject* InOuter)
	: AActor(InClass, InName, InOuter)
{
}

void ASphereActor::PostSpawnInitialize()
{
	PrimitiveComponent = new USphereComponent();
	AddOwnedComponent(PrimitiveComponent);

	if (bUseRandomColor)
	{
		RandomColorComponent = new URandomColorComponent();
		AddOwnedComponent(RandomColorComponent);
	}

	AActor::PostSpawnInitialize();
}
