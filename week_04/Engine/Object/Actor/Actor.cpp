#include "Actor.h"

#include "Object/Class.h"

namespace
{
	UObject* CreateAActorInstance(UObject* InOuter, const FString& InName)
	{
		return new AActor(AActor::StaticClass(), InName, InOuter);
	}

	FVector GZeroVector{};
}

UClass* AActor::StaticClass()
{
	static UClass ClassInfo("AActor", UObject::StaticClass(), &CreateAActorInstance);
	return &ClassInfo;
}

AActor::AActor(UClass* InClass, const FString& InName, UObject* InOuter)
	: UObject(InClass, InName, InOuter)
{
}

void AActor::AddOwnedComponent(UActorComponent* InComponent)
{
	if (InComponent == nullptr)
	{
		return;
	}

	auto It = std::find(OwnedComponents.begin(), OwnedComponents.end(), InComponent);
	if (It != OwnedComponents.end())
	{
		return;
	}

	OwnedComponents.push_back(InComponent);
	InComponent->SetOwner(this);

	if (RootComponent == nullptr && InComponent->IsA(USceneComponent::StaticClass()))
}

void AActor::RemoveOwnedComponent(UActorComponent* InComponent)
{
}

void AActor::PostSpawnInitialize()
{
}

void AActor::BeginPlay()
{
}

void AActor::Tick(float DeltaTime)
{
}

void AActor::EndPlay()
{
}

void AActor::Destroy()
{
}

const FVector& AActor::GetActorLocation() const
{
}

void AActor::SetActorLocation(const FVector& InLocation)
{
}
