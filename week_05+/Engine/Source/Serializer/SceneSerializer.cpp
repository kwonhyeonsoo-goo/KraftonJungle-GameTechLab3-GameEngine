#include "SceneSerializer.h"
#include "ThirdParty/nlohmann/json.hpp"
#include "Renderer/Material.h"
#include "Renderer/MaterialManager.h"
#include "Component/CameraComponent.h"
#include "Camera/Camera.h"
#include "Core/Paths.h"
#include "Actor/Actor.h"
#include "Actor/AttachTestActor.h"
#include "Renderer/Renderer.h"
#include "Component/PrimitiveComponent.h"
#include "World/Level.h"
#include "Object/ObjectFactory.h" 
#include "Serializer/Archive.h"
#include "Object/Class.h"
#include "Actor/StaticMeshActor.h"
#include "Component/StaticMeshComponent.h"
#include <iomanip>
#include <filesystem>
#include <fstream>
void FSceneSerializer::Save(ULevel* Level, const FString& FilePath, const FCamera* PerspectiveCamera)
{
	nlohmann::json Json;
	FCamera* Camera = Level->GetCamera();
	if (Camera)
	{
		const FVector Position = Camera->GetPosition();
		Json["Camera"]["Position"] = { Position.X, Position.Y, Position.Z };
		Json["Camera"]["Rotation"] = { Camera->GetYaw(), Camera->GetPitch() };
	}

	if (PerspectiveCamera && PerspectiveCamera->GetProjectionMode() == ECameraProjectionMode::Perspective)
	{
		const FVector Location = PerspectiveCamera->GetPosition();
		Json["PerspectiveCamera"]["Location"] = { Location.X, Location.Y, Location.Z };
		Json["PerspectiveCamera"]["Rotation"] = { PerspectiveCamera->GetYaw(), PerspectiveCamera->GetPitch() };
		Json["PerspectiveCamera"]["FOV"] = PerspectiveCamera->GetFOV();
		Json["PerspectiveCamera"]["NearClip"] = PerspectiveCamera->GetNearClip();
		Json["PerspectiveCamera"]["FarClip"] = PerspectiveCamera->GetFarClip();
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
			FString Rel = FPaths::ToRelativePath(AbsPath);
			Materials.push_back(Rel);
		}
		Json["Materials"] = Materials;
	}

	// Primitives
	nlohmann::json Primitives;
	int32 Index = 0;
	for (AActor* Actor : Level->GetActors())
	{
		if (!Actor || Actor->IsPendingDestroy())
			continue;
		if (!Actor->GetRootComponent())
			continue;
		FArchive Ar(true);
		Actor->Serialize(Ar);
		nlohmann::json& ActorJson 
			= *static_cast<nlohmann::json*>(Ar.GetRawJson());
		Primitives[std::to_string(Index)] = ActorJson;
		Index++;
	}

	Json["Primitives"] = Primitives;
	Json["NextUUID"] = FObjectFactory::GetLastUUID();
	std::ofstream File(FPaths::ToWide(FilePath));
	if (File.is_open())
	{
		File << std::setw(2) << Json << std::endl;
	}
}
bool FSceneSerializer::Load(ULevel* Level, const FString& FilePath, ID3D11Device* Device, FCamera* PerspectiveCamera)
{
	std::ifstream File(FPaths::ToWide(FilePath));
	if (!File.is_open()) return false;

	nlohmann::json Json;
	try { File >> Json; }
	catch (const std::exception& e) { return false; }

	if (!Json.contains("Primitives")) return false;

	// 매테리얼 프리로드
	if (Json.contains("Materials")) 
	{
		for (const auto& MatPath : Json["Materials"])
		{
			FString FullPath = FPaths::ToAbsolutePath(MatPath.get<std::string>());
			if (GRenderer && GRenderer->GetRenderStateManager())
			{
				FMaterialManager::Get().LoadFromFile(Device, GRenderer->GetRenderStateManager().get(), FullPath);
			}
		}
	}

	// -------------------------------------------------------------------
	// [안전 장치] JSON 값이 float인지, [float] 배열인지 유연하게 파싱하는 람다
	// -------------------------------------------------------------------
	auto GetFloatSafe = [](const nlohmann::json& node) -> float {
		if (node.is_array() && !node.empty()) return node[0].get<float>();
		if (node.is_number()) return node.get<float>();
		return 0.0f;
	};

	// 기본 카메라 파싱
	FCamera* Camera = Level->GetCamera();
	if (Camera && Json.contains("Camera"))
	{
		auto& Cam = Json["Camera"];
		if (Cam.contains("Position"))
		{
			Camera->SetPosition({ Cam["Position"][0].get<float>(), Cam["Position"][1].get<float>(), Cam["Position"][2].get<float>() });
		}
		if (Cam.contains("Rotation"))
		{
			Camera->SetRotation(Cam["Rotation"][0].get<float>(), Cam["Rotation"][1].get<float>());
		}
	}

	// 원근 카메라 파싱
	if (PerspectiveCamera && Json.contains("PerspectiveCamera"))
	{
		auto& PCamJson = Json["PerspectiveCamera"];
		if (PCamJson.contains("Location"))
		{
			PerspectiveCamera->SetPosition({ PCamJson["Location"][0].get<float>(), PCamJson["Location"][1].get<float>(), PCamJson["Location"][2].get<float>() });
		}
		if (PCamJson.contains("Rotation") && PCamJson["Rotation"].size() >= 2)
		{
			PerspectiveCamera->SetRotation(PCamJson["Rotation"][0].get<float>(), PCamJson["Rotation"][1].get<float>());
		}
		
		if (PCamJson.contains("FOV"))       PerspectiveCamera->SetFOV(GetFloatSafe(PCamJson["FOV"]));
		if (PCamJson.contains("NearClip"))  PerspectiveCamera->SetNearClip(GetFloatSafe(PCamJson["NearClip"]));
		
	}

	// Primitives 파싱
	int32 ActorIndex = 0;
	auto Items = Json["Primitives"].items();
	for (auto& [Key, Value] : Items)
	{
	
		FString ClassName = Value.value("Class", "");
		if (ClassName.empty())
		{
			ClassName = Value.value("Type", "");
		}

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
		
		// 직렬화 로드
		FArchive Ar(false);
		*static_cast<nlohmann::json*>(Ar.GetRawJson()) = Value;
		Actor->Serialize(Ar);

		++ActorIndex;
	}

	// UUID 복구
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