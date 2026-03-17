#include "AttachTestActor.h"
#include "Component/SphereComponent.h"
#include "Component/CubeComponent.h"

namespace
{
	UObject* CreateAAttachTestActorInstance(UObject* InOuter, const FString& InName)
	{
		return new AAttachTestActor(AAttachTestActor::StaticClass(), InName, InOuter);
	}
}

UClass* AAttachTestActor::StaticClass()
{
	static UClass ClassInfo("AAttachTestActor", AActor::StaticClass(), &CreateAAttachTestActorInstance);
	return &ClassInfo;
}

AAttachTestActor::AAttachTestActor(UClass* InClass, const FString& InName, UObject* InOuter)
	: AActor(InClass, InName, InOuter)
{
}

void AAttachTestActor::PostSpawnInitialize()
{
	SphereComponent = new USphereComponent();
	CubeComponent = new UCubeComponent();

	CubeComponent->AttachTo(SphereComponent);
	CubeComponent->SetRelativeTransform({ FRotator::MakeFromEuler({ 45.0f, 45.0f, 45.0f }), {0.0f, 0.0f, 2.0f}, {0.5f, 0.5f, 0.5f} });

	AddOwnedComponent(SphereComponent);
	AddOwnedComponent(CubeComponent);

	AActor::PostSpawnInitialize();
}
