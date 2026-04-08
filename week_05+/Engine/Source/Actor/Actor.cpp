#include "Actor.h"
#include "Object/ObjectFactory.h"
#include "Component/BillboardComponent.h"
#include "Object/Class.h"
#include "Renderer/Material.h"
#include "Component/TextRenderComponent.h"
#include "Component/SceneComponent.h"
#include "Serializer/Archive.h"
#include "Component/StaticMeshComponent.h"
#include "ThirdParty/nlohmann/json.hpp"

#include "Serializer/Archive.h"
#include "World/Level.h"
#include <unordered_map>

IMPLEMENT_RTTI(AActor, UObject)

ULevel* AActor::GetLevel() const { return Level; }
void AActor::SetLevel(ULevel* InLevel) { Level = InLevel; }
UWorld* AActor::GetWorld() const
{
	if (Level)
	{
		return Level->GetTypedOuter<UWorld>();
	}
	return nullptr;
}
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
	if (GetComponentByClass<UTextRenderComponent>() == nullptr)
	{
		UTextRenderComponent* TextComponent =
			FObjectFactory::ConstructObject<UTextRenderComponent>(this, "TextComponent");

		if (TextComponent)
		{
			AddOwnedComponent(TextComponent);
			if (RootComponent && RootComponent != TextComponent)
			{
				TextComponent->AttachTo(RootComponent);
			}

			TextComponent->SetWorldOffset(FVector(0.0f, 0.0f, 0.5f));
			TextComponent->SetWorldScale(0.3f);
			TextComponent->SetTextColor(FVector4(1.0f, 1.0f, 1.0f, 1.0f));
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

void AActor::Serialize(FArchive& Ar)
{
	UObject::Serialize(Ar);

	if (Ar.IsSaving()) // Save Actor property
	{
		FString ClassName = GetClass()->GetName();
		Ar.Serialize("Class", ClassName);
		Ar.Serialize("UUID", UUID);

		TArray<uint32> CompUUIDs;
		for (UActorComponent* Comp : GetComponents())
			if (Comp)
			{
				Comp->SetOwner(this);

				if (RootComponent && Comp != RootComponent && Comp->IsA(USceneComponent::StaticClass()))
				{
					static_cast<USceneComponent*>(Comp)->AttachTo(RootComponent);
				}
			}
		Ar.SerializeUIntArray("ComponentUUIDs", CompUUIDs);
	}
	else // Load 
	{
		if (Ar.Contains("UUID"))
		{
			uint32 SavedUUID = 0;
			Ar.Serialize("UUID", SavedUUID);
			// 기존 UUID 제거
			GUUIDToObjectMap.erase(UUID);
			// 충돌하는 UUID가 이미 있으면 기존 것 제거
			if (auto It = GUUIDToObjectMap.find(SavedUUID); It != GUUIDToObjectMap.end() && It->second != this)
			{
				It->second->UUID = 0;
				GUUIDToObjectMap.erase(It);
			}
			UUID = SavedUUID;
			GUUIDToObjectMap[SavedUUID] = this;
		}

		// Components UUID Restore
		if (Ar.Contains("ComponentUUIDs"))
		{
			TArray<uint32> CompUUIDs;
			Ar.SerializeUIntArray("ComponentUUIDs", CompUUIDs);
			const TArray<UActorComponent*>& Components = GetComponents();
			for (size_t i = 0; i < CompUUIDs.size(); i++)
			{
				GUUIDToObjectMap.erase(Components[i]->UUID);
				if (auto It = GUUIDToObjectMap.find(CompUUIDs[i]);
					It != GUUIDToObjectMap.end() &&
					It->second != Components[i])
				{
					It->second->UUID = 0;
					GUUIDToObjectMap.erase(It);
				}
				Components[i]->UUID = CompUUIDs[i];
				GUUIDToObjectMap[CompUUIDs[i]] = Components[i];
			}
		}

		// Setting Owner
		for (UActorComponent* Comp : GetComponents())
			if (Comp)
				Comp->SetOwner(this);
	}
	// 컴포넌트 데이터 직렬화
	if (Ar.IsSaving())
	{
		auto& ParentJson = *static_cast<nlohmann::json*>(Ar.GetRawJson());
		nlohmann::json ComponentsJson = nlohmann::json::array();
		for (UActorComponent* Comp : GetComponents())
		{
			if (Comp)
			{
				FArchive CompAr(true);
				Comp->Serialize(CompAr);
				ComponentsJson.push_back(*static_cast<nlohmann::json*>(CompAr.GetRawJson()));
			}
		}
		ParentJson["ComponentsData"] = ComponentsJson;
	}
	else
	{
		auto& ParentJson = *static_cast<nlohmann::json*>(Ar.GetRawJson());
		if (ParentJson.contains("ComponentsData") && ParentJson["ComponentsData"].is_array())
		{
			const auto& ComponentsJson = ParentJson["ComponentsData"];
			const TArray<UActorComponent*>& Components = GetComponents();
			size_t Count = std::min((size_t)Components.size(), ComponentsJson.size());
			for (size_t i = 0; i < Count; ++i)
			{
				if (Components[i])
				{
					FArchive CompAr(false);
					*static_cast<nlohmann::json*>(CompAr.GetRawJson()) = ComponentsJson[i];
					Components[i]->Serialize(CompAr);
				}
			}
		}
	}
}
const FVector& AActor::GetActorLocation() const
{
	if (RootComponent == nullptr)
	{
		return FVector::ZeroVector;
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

void AActor::DuplicateSubObjects()
{
	UObject::DuplicateSubObjects();

	TArray<UActorComponent*> OldComponents = OwnedComponents;
	USceneComponent* OldRoot = RootComponent;

	OwnedComponents.clear();
	RootComponent = nullptr;

	std::unordered_map<UActorComponent*, UActorComponent*> CompMap;

	for (UActorComponent* Component : OldComponents)
	{
		if (Component)
		{
			UActorComponent* DuplicatedComp = static_cast<UActorComponent*>(Component->Duplicate());
			DuplicatedComp->SetOwner(this);
			OwnedComponents.push_back(DuplicatedComp);

			CompMap[Component] = DuplicatedComp;

			if (Component == OldRoot)
			{
				RootComponent = static_cast<USceneComponent*>(DuplicatedComp);
			}
		}
	}

	for (UActorComponent* Component : OldComponents)
	{
		USceneComponent* OldSceneComp = dynamic_cast<USceneComponent*>(Component);
		if (OldSceneComp && OldSceneComp->GetAttachParent())
		{
			if (CompMap.count(Component) && CompMap.count(OldSceneComp->GetAttachParent()))
			{
				USceneComponent* NewSceneComp = static_cast<USceneComponent*>(CompMap[Component]);
				USceneComponent* NewParent = static_cast<USceneComponent*>(CompMap[OldSceneComp->GetAttachParent()]);
				if (NewSceneComp && NewParent)
				{
					NewSceneComp->AttachTo(NewParent);
				}
			}
		}
	}
}
