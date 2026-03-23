#include "SceneSerializer.h"
#include "ThirdParty/nlohmann/json.hpp"
#include "Renderer/Material.h"
#include "Renderer/MaterialManager.h"
#include "Component/CameraComponent.h"
#include "Camera/Camera.h"
#include "Core/Paths.h"
#include "Actor/Actor.h"
#include "Actor/AttachTestActor.h"
#include "Actor/CubeActor.h"
#include "Actor/SphereActor.h"
#include "Component/PrimitiveComponent.h"

#include "Object/ObjectFactory.h" 
#include <iomanip>
#include <filesystem>
#include <fstream>
void FSceneSerializer::Save(UScene* Scene, const FString& FilePath)
{
	nlohmann::json Json;
	CCamera* Camera = Scene->GetCamera();
	if (Camera)
	{
		const FVector Position = Camera->GetPosition();
		Json["Camera"]["Position"] = { Position.X, Position.Y, Position.Z };
		Json["Camera"]["Rotation"] = { Camera->GetYaw(), Camera->GetPitch() };
	}

	// Materials (로드된 Material 파일 경로를 프로젝트 루트 기준 상대 경로로 저장)
	TArray<FString> LoadedPaths = FMaterialManager::Get().GetLoadedPaths();
	if (!LoadedPaths.empty())
	{
		nlohmann::json Materials = nlohmann::json::array();
		FString Root = FPaths::ProjectRoot().string();
		for (const FString& AbsPath : LoadedPaths)
		{
			// 절대 경로 → 프로젝트 루트 기준 상대 경로
			std::filesystem::path Rel = std::filesystem::relative(AbsPath, Root);
			Materials.push_back(Rel.generic_string());
		}
		Json["Materials"] = Materials;
	}

	// Primitives
	nlohmann::json Primitives;
	int32 Index = 0;
	for (AActor* Actor : Scene->GetActors())
	{
		if (!Actor || Actor->IsPendingDestroy())
		{
			continue;
		}

		FString Type;
		if (Actor->IsA(ASphereActor::StaticClass()))
		{
			Type = "Sphere";
		}
		else if (Actor->IsA(ACubeActor::StaticClass()))
		{
			Type = "Cube";
		}
		else if (Actor->IsA(AAttachTestActor::StaticClass()))
		{
			Type = "AttachTest";
		}
		else
		{
			continue;
		}
		USceneComponent* Root = Actor->GetRootComponent();
		if (!Root)
		{
			continue;
		}
		USceneComponent* Root = Actor->GetRootComponent();
		if (!Root)
		{
			continue;
		}
		const FTransform Transform = Root->GetRelativeTransform();
		const FVector EulerDegrees = Transform.Rotator().Euler();
		const FString Key = std::to_string(Index);

		Primitives[Key]["Type"] = Type;
		Primitives[Key]["UUID"] = Actor->UUID;
		nlohmann::json CompUUIDs = nlohmann::json::array();
		for (UActorComponent* Comp : Actor->GetComponents())
		{
			if (Comp)
			{
				CompUUIDs.push_back(Comp->UUID);
			}
		}
		Primitives[Key]["ComponentUUIDs"] = CompUUIDs;
		// Material 이름 저장 (에셋 원본 이름 사용)
		UPrimitiveComponent* PrimComp = Actor->GetComponentByClass<UPrimitiveComponent>();
		if (PrimComp && PrimComp->GetMaterial() && !PrimComp->GetMaterial()->GetOriginName().empty())
		{
			Primitives[Key]["Material"] = PrimComp->GetMaterial()->GetOriginName();
		}

		Primitives[Key]["Location"] = {
			Transform.GetTranslation().X,
			Transform.GetTranslation().Y,
			Transform.GetTranslation().Z
		};
		Primitives[Key]["Rotation"] = { EulerDegrees.X, EulerDegrees.Y, EulerDegrees.Z };
		Primitives[Key]["Scale"] = {
			Transform.GetScale3D().X,
			Transform.GetScale3D().Y,
			Transform.GetScale3D().Z
		};

		++Index;
	}

	Json["Primitives"] = Primitives;
	Json["NextUUID"] = FObjectFactory::GetLastUUID();
	std::ofstream File(FilePath);
	if (File.is_open())
	{
		File << std::setw(2) << Json << std::endl;
	}
}

void FSceneSerializer::Load(UScene* Scene, const FString& FilePath, ID3D11Device* Device)
{
	std::ifstream File(FilePath);
	if (!File.is_open())
	{
		return;
	}

	nlohmann::json Json;
	File >> Json;
	CCamera* Camera = Scene->GetCamera();
	if (Camera && Json.contains("Camera"))
	{
		auto& Cam = Json["Camera"];
		if (Cam.contains("Position"))
		{
			auto& P = Cam["Position"];
			Camera->SetPosition({ P[0].get<float>(), P[1].get<float>(), P[2].get<float>() });
		}
		if (Cam.contains("Rotation"))
		{
			auto& R = Cam["Rotation"];
			Camera->SetRotation(R[0].get<float>(), R[1].get<float>());
		}
	}

	if (Device && Json.contains("Materials"))
	{
		for (auto& MatPath : Json["Materials"])
		{
			const FString RelativePath = MatPath.get<FString>();
			const FString AbsolutePath = (FPaths::ProjectRoot() / RelativePath).string();
			FMaterialManager::Get().GetOrLoad(Device, AbsolutePath);
		}
	}

	if (!Json.contains("Primitives"))
	{
		return;
	}


	int32 ActorIndex = 0;
	for (auto& [Key, Value] : Json["Primitives"].items())
	{
		const FString Type = Value.value("Type", "");
		const FString ActorName = Type + "_" + std::to_string(ActorIndex);

		AActor* Actor = nullptr;
		if (Type == "Sphere")
		{
			Actor = Scene->SpawnActor<ASphereActor>(ActorName);
		}
		else if (Type == "Cube")
		{
			Actor = Scene->SpawnActor<ACubeActor>(ActorName);
		}
		else if (Type == "AttachTest")
		{
			Actor = Scene->SpawnActor<AAttachTestActor>(ActorName);
		}
		else
		{
			++ActorIndex;
			continue;
		}
		if (Value.contains("UUID"))
		{
			uint32 SavedUUID = Value["UUID"].get<uint32>();
			// 기존 UUID 제거
			GUUIDToObjectMap.erase(Actor->UUID);
			// 충돌하는 UUID가 이미 있으면 기존 것 제거
			if (auto It = GUUIDToObjectMap.find(SavedUUID); It != GUUIDToObjectMap.end() && It->second != Actor)
			{
				It->second->UUID = 0;
				GUUIDToObjectMap.erase(It);
			}
			Actor->UUID = SavedUUID;
			GUUIDToObjectMap[SavedUUID] = Actor;
		}

		if (Value.contains("Material"))
		{
			const FString MaterialName = Value["Material"].get<FString>();
			const std::shared_ptr<FMaterial> Material = FMaterialManager::Get().FindByName(MaterialName);
			if (Material)
			{
				if (UPrimitiveComponent* PrimitiveComponent = Actor->GetComponentByClass<UPrimitiveComponent>())
				{
					PrimitiveComponent->SetMaterial(Material.get());
				}
			}
		}

		FTransform Transform;
		if (Value.contains("Location"))
		{
			auto& L = Value["Location"];
			Transform.SetTranslation({ L[0].get<float>(), L[1].get<float>(), L[2].get<float>() });
		}
		if (Value.contains("Rotation"))
		{
			auto& R = Value["Rotation"];
			const FVector EulerDegrees(R[0].get<float>(), R[1].get<float>(), R[2].get<float>());
			Transform.SetRotation(FRotator::MakeFromEuler(EulerDegrees));
		}
		if (Value.contains("Scale"))
		{
			auto& S = Value["Scale"];
			Transform.SetScale3D({ S[0].get<float>(), S[1].get<float>(), S[2].get<float>() });
		}

		if (USceneComponent* Root = Actor->GetRootComponent())
		{
			Root->SetRelativeTransform(Transform);
		}
		if (Value.contains("ComponentUUIDs"))
		{
			auto& CompUUIDs = Value["ComponentUUIDs"];
			const auto& Components = Actor->GetComponents();
			for (size_t i = 0; i < CompUUIDs.size() && i < Components.size(); ++i)
			{
				uint32 SavedCompUUID = CompUUIDs[i].get<uint32>();
				GUUIDToObjectMap.erase(Components[i]->UUID);
				if (auto It = GUUIDToObjectMap.find(SavedCompUUID); It != GUUIDToObjectMap.end() && It->second != Components[i])
				{
					It->second->UUID = 0;
					GUUIDToObjectMap.erase(It);
				}
				Components[i]->UUID = SavedCompUUID;
				GUUIDToObjectMap[SavedCompUUID] = Components[i];
			}
		}

		//AddOwnedComponent → SetOwner(this) → TObjectPtr이 Actor의 UUID를 캐싱
		//	그 다음 LoadSceneFromFile에서 Actor->UUID = SavedUUID로 UUID 변경
		//	TObjectPtr은 여전히 이전 UUID를 기억
		//	GetOwner() → TObjectPtr이 캐싱된 UUID로 맵 조회 → 맵에 없음 → nullptr 반환
		// 해결안 UUID 변경 후에 Owner를 다시 설정
		for (UActorComponent* Comp : Actor->GetComponents())
		{
			if (Comp)
			{
				Comp->SetOwner(Actor);
			}
		}
		++ActorIndex;

	}
	if (Json.contains("NextUUID"))
	{
		uint32 Saved = Json["NextUUID"].get<uint32>();
		if (Saved > FObjectFactory::GetLastUUID())
		{
			FObjectFactory::SetLastUUID(Saved);
		}
	}
}
