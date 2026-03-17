#include "SphereActor.h"
#include "Component/SphereComponent.h"
#include "Component/RandomColorComponent.h"
#include "Object/ObjectFactory.h"

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
	PrimitiveComponent = FObjectFactory::ConstructObject<USphereComponent>(this);
	AddOwnedComponent(PrimitiveComponent);

	if (bUseRandomColor)
	{
		RandomColorComponent = FObjectFactory::ConstructObject<URandomColorComponent>(this);
		AddOwnedComponent(RandomColorComponent);
	}

	AActor::PostSpawnInitialize();
}
