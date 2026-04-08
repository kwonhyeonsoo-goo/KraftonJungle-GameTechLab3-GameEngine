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
			if (RootComponent)
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
	// RootComponent 직렬화 
	// 만약 루트가 StaticMeshComponent라면, 위치/회전/크기부터 메쉬/매테리얼/텍스처까지 연쇄적으로 싹 다 자동 저장&로드
	if (USceneComponent* Root = GetRootComponent())
	{
		if (Ar.IsSaving())
		{
			FArchive CompAr(true); // RootComponent 전용 빈 도화지 생성
			Root->Serialize(CompAr); // 여기에 마음껏 쓰게 함

			// 다 쓴 도화지를 액터 도화지의 "RootComponent" 폴더에 통째
			(*static_cast<nlohmann::json*>(Ar.GetRawJson()))["RootComponent"] = *static_cast<nlohmann::json*>(CompAr.GetRawJson());
		}
		else // IsLoading
		{
			auto& ParentJson = *static_cast<nlohmann::json*>(Ar.GetRawJson());
			if (ParentJson.contains("RootComponent"))
			{
				FArchive CompAr(false);
				*static_cast<nlohmann::json*>(CompAr.GetRawJson()) = ParentJson["RootComponent"];
				Root->Serialize(CompAr);
			}
			else
			{
				Root->Serialize(Ar);
			}
		}
	}

	// 2. TextComponent 직렬화 (UUIDBillboardComponent와 충돌하지 않도록 필터링)
	UTextRenderComponent* RealTC = nullptr;
	for (UActorComponent* Comp : GetComponents())
	{
		// UUID빌보드가 아닌 진짜 순수 TextComponent만 
		if (Comp->IsA(UTextRenderComponent::StaticClass()) && !Comp->IsA(UBillboardComponent::StaticClass()))
		{
			RealTC = static_cast<UTextRenderComponent*>(Comp);
			break;
		}
	}

	if (RealTC)
	{
		if (Ar.IsSaving())
		{
			FArchive CompAr(true);
			RealTC->Serialize(CompAr);
			(*static_cast<nlohmann::json*>(Ar.GetRawJson()))["TextComponent"] = *static_cast<nlohmann::json*>(CompAr.GetRawJson());
		}
		else
		{
			auto& ParentJson = *static_cast<nlohmann::json*>(Ar.GetRawJson());
			if (ParentJson.contains("TextComponent"))
			{
				FArchive CompAr(false);
				*static_cast<nlohmann::json*>(CompAr.GetRawJson()) = ParentJson["TextComponent"];
				RealTC->Serialize(CompAr);
			}
		}
	}
	UBillboardComponent* RealBC = nullptr;
	for (UActorComponent* Comp : GetComponents())
	{
		if (Comp->IsA(UBillboardComponent::StaticClass()))
		{
			RealBC = static_cast<UBillboardComponent*>(Comp);
			break;
		}
	}

	if (RealBC)
	{
		if (Ar.IsSaving())
		{
			FArchive CompAr(true);
			RealBC->Serialize(CompAr);
			(*static_cast<nlohmann::json*>(Ar.GetRawJson()))["BillboardComponent"] = *static_cast<nlohmann::json*>(CompAr.GetRawJson());
		}
		else
		{
			auto& ParentJson = *static_cast<nlohmann::json*>(Ar.GetRawJson());
			if (ParentJson.contains("BillboardComponent"))
			{
				FArchive CompAr(false);
				*static_cast<nlohmann::json*>(CompAr.GetRawJson()) = ParentJson["BillboardComponent"];
				RealBC->Serialize(CompAr);
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

	for (UActorComponent* Component : OldComponents)
	{
		if (Component)
		{
			UTextRenderComponent* UUIDComp = dynamic_cast<UTextRenderComponent*>(Component);
			if (UUIDComp)
			{
				continue; // UUIDBillboardComponent는 복제하지 않음
			}

			UActorComponent* DuplicatedComp = static_cast<UActorComponent*>(Component->Duplicate());
			DuplicatedComp->SetOwner(this);
			OwnedComponents.push_back(DuplicatedComp);

			if (Component == OldRoot)
			{
				RootComponent = static_cast<USceneComponent*>(DuplicatedComp);
			}
		}
	}
}
