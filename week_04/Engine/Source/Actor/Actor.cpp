#include "Actor.h"
#include "Object/ObjectFactory.h"
#include "Component/UUIDBillboardComponent.h"
#include "Object/Class.h"
#include "Component/SceneComponent.h"

IMPLEMENT_RTTI(AActor, UObject)

namespace {
	FVector GZeroVector{};
}

UScene* AActor::GetScene() const { return Scene; }
void AActor::SetScene(UScene* InScene) { Scene = InScene; }
USceneComponent* AActor::GetRootComponent() const { return RootComponent; }

void AActor::SetRootComponent(USceneComponent* InRootComponent)
{
	// 의문점
	// 기존에 RootComponent가 있을 시에는 RootComponent의 OwnerActor를 지워주나?
	// 이러면 두 개의 RootComponent가 하나의 Owner을 가지고 있는건데.
	RootComponent = InRootComponent;
	if (RootComponent)
	{
		RootComponent->SetOwner(this);
	}
}

const TArray<UActorComponent*>& AActor::GetComponents() const { return OwnedComponents; }

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
	{
		RootComponent = static_cast<USceneComponent*>(InComponent);
	}
}

void AActor::RemoveOwnedComponent(UActorComponent* InComponent)
{
	if (InComponent == nullptr)
	{
		return;
	}

	std::erase(OwnedComponents, InComponent);

	if (RootComponent == InComponent)
	{
		RootComponent = nullptr;
	}

	InComponent->SetOwner(nullptr);
}

void AActor::PostSpawnInitialize()
{
	if (GetComponentByClass<UUUIDBillboardComponent>() == nullptr)
	{
		UUUIDBillboardComponent* UUIDComponent =
			FObjectFactory::ConstructObject<UUUIDBillboardComponent>(this, "UUIDBillboard");

		if (UUIDComponent)
		{
			AddOwnedComponent(UUIDComponent);

			UUIDComponent->SetWorldOffset(FVector(0.0f, 0.0f, 0.3f));
			UUIDComponent->SetWorldScale(0.3f);
			UUIDComponent->SetTextColor(FVector4(1.0f, 1.0f, 1.0f, 1.0f));
		}
	}

	for (UActorComponent* Component : OwnedComponents)
	{
		if (Component && !Component->IsRegistered())
		{
			Component->OnRegister();
		}
	}
}

void AActor::BeginPlay()
{
	if (bActorBegunPlay)
	{
		return;
	}

	bActorBegunPlay = true;

	for (UActorComponent* Component : OwnedComponents)
	{
		if (Component && !Component->HasBegunPlay())
		{
			Component->BeginPlay();
		}
	}
}

void AActor::Tick(float DeltaTime)
{
	if (!CanTick() || bPendingDestroy)
	{
		return;
	}

	for (UActorComponent* Component : OwnedComponents)
	{
		if (Component && Component->CanTick())
		{
			Component->Tick(DeltaTime);
		}
	}
}

void AActor::EndPlay()
{
}

void AActor::Destroy()
{
	if (bPendingDestroy)
	{
		return;
	}

	bPendingDestroy = true;
	MarkPendingKill();

	for (UActorComponent* Comp : OwnedComponents)
	{
		if (Comp)
		{
			Comp->MarkPendingKill();
		}
	}
}

const FVector& AActor::GetActorLocation() const
{
	if (RootComponent == nullptr)
	{
		return GZeroVector;
	}

	return RootComponent->GetRelativeLocation();
}

void AActor::SetActorLocation(const FVector& InLocation)
{
	if (RootComponent == nullptr)
	{
		return;
	}

	RootComponent->SetRelativeLocation(InLocation);
}
