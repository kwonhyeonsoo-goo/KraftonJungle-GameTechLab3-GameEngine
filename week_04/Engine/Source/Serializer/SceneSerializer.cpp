#include "SceneSerializer.h"
#include "ThirdParty/nlohmann/json.hpp"
#include "Renderer/Material.h"
#include "Renderer/MaterialManager.h"
#include "Component/CameraComponent.h"
#include "Core/Paths.h"
#include "Actor/Actor.h"
#include "Actor/AttachTestActor.h"
#include "Actor/CubeActor.h"
#include "Actor/SphereActor.h"
#include "Actor/ObjActor.h"
#include "Component/PrimitiveComponent.h"
#include "World/Level.h"
#include "Object/ObjectFactory.h"
#include "Serializer/Archive.h"
#include "Object/Class.h"
#include <iomanip>
#include <filesystem>
#include <fstream>

void FSceneSerializer::Save(ULevel* Level, const FString& FilePath, UCameraComponent* Camera)
{
	nlohmann::json Json;

	// 카메라 직렬화 — UCameraComponent Transform 직접 사용
	if (Camera)
	{
		Json["Camera"]["Position"] = { Camera->GetPosition().X, Camera->GetPosition().Y, Camera->GetPosition().Z };
		Json["Camera"]["Rotation"] = { Camera->GetYaw(), Camera->GetPitch() };
		Json["Camera"]["FOV"] = Camera->GetFOV();
		Json["Camera"]["NearClip"] = Camera->GetNearPlane();
		Json["Camera"]["FarClip"] = Camera->GetFarPlane();
	}

	// Materials
	TArray<FString> LoadedPaths = FMaterialManager::Get().GetLoadedPaths();
	if (!LoadedPaths.empty())
	{
		nlohmann::json Materials = nlohmann::json::array();
		FString Root = FPaths::FromPath(FPaths::ProjectRoot());
		for (const FString& AbsPath : LoadedPaths)
		{
			std::filesystem::path Rel = std::filesystem::relative(AbsPath, Root);
			Materials.push_back(Rel.generic_string());
		}
		Json["Materials"] = Materials;
	}

	// Primitives
	nlohmann::json Primitives;
	int32 Index = 0;
	for (AActor* Actor : Level->GetActors())
	{
		if (!Actor || Actor->IsPendingDestroy()) continue;
		if (!Actor->GetRootComponent()) continue;

		FArchive Ar(true);
		Actor->Serialize(Ar);
		nlohmann::json& ActorJson = *static_cast<nlohmann::json*>(Ar.GetRawJson());
		Primitives[std::to_string(Index)] = ActorJson;
		Index++;
	}

	Json["Primitives"] = Primitives;
	Json["NextUUID"] = FObjectFactory::GetLastUUID();

	std::ofstream File(FilePath);
	if (File.is_open())
	{
		File << std::setw(2) << Json << std::endl;
	}
}

bool FSceneSerializer::Load(ULevel* Level, const FString& FilePath, ID3D11Device* Device, UCameraComponent* Camera)
{
	std::ifstream File(FilePath);
	if (!File.is_open())
	{
		return false;
	}

	nlohmann::json Json;
	try
	{
		File >> Json;
	}
	catch (const std::exception&)
	{
		return false;
	}

	if (!Json.contains("Primitives"))
	{
		return false;
	}

	// 카메라 복원 — UCameraComponent Transform에 직접 적용
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
		if (Cam.contains("FOV"))
		{
			Camera->SetFOV(Cam["FOV"].get<float>());
		}
		if (Cam.contains("NearClip"))
		{
			Camera->SetNearPlane(Cam["NearClip"].get<float>());
		}
		if (Cam.contains("FarClip"))
		{
			Camera->SetFarPlane(Cam["FarClip"].get<float>());
		}
	}

	// Primitives
	int32 ActorIndex = 0;
	for (auto& [Key, Value] : Json["Primitives"].items())
	{
		FString ClassName = Value.value("Class", "");
		UClass* ActorClass = UClass::FindClass(ClassName);
		if (!ActorClass)
		{
			ActorIndex++;
			continue;
		}

		const FString ActorName = ClassName + "_" + std::to_string(ActorIndex);
		AActor* Actor = static_cast<AActor*>(FObjectFactory::ConstructObject(ActorClass, Level, ActorName));
		if (!Actor)
		{
			ActorIndex++;
			continue;
		}

		Level->RegisterActor(Actor);
		Actor->PostSpawnInitialize();

		FArchive Ar(false);
		*static_cast<nlohmann::json*>(Ar.GetRawJson()) = Value;
		Actor->Serialize(Ar);

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

		if (Value.contains("PrimitiveFileName") && Actor->IsA(AObjActor::StaticClass()))
		{
			const FString PrimitiveFileName = Value["PrimitiveFileName"].get<FString>();
			if (!PrimitiveFileName.empty())
			{
				if (AObjActor* ObjActor = static_cast<AObjActor*>(Actor))
				{
					ObjActor->LoadObj(Device, PrimitiveFileName);
				}
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

	return true;
}