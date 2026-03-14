#include "Scene.h"

#include "Core/Core.h"
#include "Object/Class.h"
#include "Object/Actor/Actor.h"
#include "Camera/Camera.h"
#include "Component/SphereComponent.h"
#include "Component/CubeComponent.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include "ThirdParty/nlohmann/json.hpp"
#include "Component/PrimitiveComponent.h"

namespace
{
	UObject* CreateUWorldInstance(UObject* InOuter, const FString& InName)
	{
		return new UScene(UScene::StaticClass(), InName, InOuter);
	}
}

UClass* UScene::StaticClass()
{
	static UClass ClassInfo("UWorld", UObject::StaticClass(), &CreateUWorldInstance);
	return &ClassInfo;
}

UScene::UScene(UClass* InClass, const FString& InName, UObject* InOuter)
	: UObject(InClass, InName, InOuter)
{
}

UScene::~UScene()
{
	for (AActor* Actor : Actors)
	{
		delete Actor;
	}
	Actors.clear();

	delete Camera;
	Camera = nullptr;
}

void UScene::InitializeDefaultScene(float AspectRatio)
{
	// 카메라
	Camera = new CCamera();
	Camera->SetAspectRatio(AspectRatio);

	// JSON 파일에서 씬 로드 (카메라 포함)
	LoadSceneFromFile("../Assets/Scenes/DefaultScene.json");
}

void UScene::LoadSceneFromFile(const FString& FilePath)
{
	std::ifstream File(FilePath);
	if (!File.is_open()) return;

	nlohmann::json Json;
	File >> Json;

	// 카메라 설정 로드
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

	if (!Json.contains("Primitives")) return;

	int ActorIndex = 0;
	for (auto& [Key, Value] : Json["Primitives"].items())
	{
		FString Type = Value.value("Type", "");

		UActorComponent* Comp = nullptr;
		if (Type == "Sphere")
		{
			Comp = new USphereComponent();
		}
		else if (Type == "Cube")
		{
			Comp = new UCubeComponent();
		}
		else
		{
			++ActorIndex;
			continue;
		}

		FString ActorName = Type + "_" + std::to_string(ActorIndex);
		AActor* Actor = SpawnActor<AActor>(ActorName);
		Actor->AddOwnedComponent(Comp);

		FTransform Transform;
		if (Value.contains("Location"))
		{
			auto& L = Value["Location"];
			Transform.Location = { L[0].get<float>(), L[1].get<float>(), L[2].get<float>() };
		}
		if (Value.contains("Rotation"))
		{
			auto& R = Value["Rotation"];
			Transform.Rotation = { R[0].get<float>(), R[1].get<float>(), R[2].get<float>() };
		}
		if (Value.contains("Scale"))
		{
			auto& S = Value["Scale"];
			Transform.Scale = { S[0].get<float>(), S[1].get<float>(), S[2].get<float>() };
		}
		Actor->GetRootComponent()->SetRelativeTransform(Transform);

		++ActorIndex;
	}
}

void UScene::SaveSceneToFile(const FString& FilePath)
{
	nlohmann::json Json;

	// Camera
	if (Camera)
	{
		FVector Pos = Camera->GetPosition();
		Json["Camera"]["Position"] = { Pos.X, Pos.Y, Pos.Z };
		Json["Camera"]["Rotation"] = { Camera->GetYaw(), Camera->GetPitch() };
	}

	// Primitives
	nlohmann::json Primitives;
	int Index = 0;
	for (AActor* Actor : Actors)
	{
		if (!Actor || Actor->IsPendingDestroy())
			continue;

		for (UActorComponent* Comp : Actor->GetComponents())
		{
			UPrimitiveComponent* PrimComp = dynamic_cast<UPrimitiveComponent*>(Comp);
			if (!PrimComp)
				continue;

			FString Type;
			if (dynamic_cast<USphereComponent*>(PrimComp))
				Type = "Sphere";
			else if (dynamic_cast<UCubeComponent*>(PrimComp))
				Type = "Cube";
			else
				continue;

			FTransform Transform = Actor->GetRootComponent()->GetRelativeTransform();
			FString Key = std::to_string(Index);

			Primitives[Key]["Type"] = Type;
			Primitives[Key]["Location"] = { Transform.Location.X, Transform.Location.Y, Transform.Location.Z };
			Primitives[Key]["Rotation"] = { Transform.Rotation.X, Transform.Rotation.Y, Transform.Rotation.Z };
			Primitives[Key]["Scale"] = { Transform.Scale.X, Transform.Scale.Y, Transform.Scale.Z };

			++Index;
		}
	}
	Json["Primitives"] = Primitives;

	std::ofstream File(FilePath);
	if (File.is_open())
	{
		File << std::setw(2) << Json << std::endl;
	}
}

void UScene::ClearActors()
{
	for (AActor* Actor : Actors)
	{
		delete Actor;
	}
	Actors.clear();
}

void UScene::RegisterActor(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}

	auto It = std::find(Actors.begin(), Actors.end(), InActor);
	if (It != Actors.end())
	{
		return;
	}

	Actors.push_back(InActor);
	InActor->SetScene(this);
}

void UScene::DestroyActor(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}

	InActor->Destroy();
}

void UScene::CleanupDestroyedActors()
{
	auto NewEnd = std::ranges::remove_if(Actors,
		[](const AActor* Actor)
		{
			return Actor == nullptr || Actor->IsPendingDestroy();
		}).begin();

	Actors.erase(NewEnd, Actors.end());
}

void UScene::BeginPlay()
{
	if (bBegunPlay)
	{
		return;
	}

	bBegunPlay = true;

	for (AActor* Actor : Actors)
	{
		if (Actor && !Actor->HasBegunPlay())
		{
			Actor->BeginPlay();
		}
	}
}

void UScene::Tick(float DeltaTime)
{
	if (!bBegunPlay)
	{
		BeginPlay();
	}

	for (AActor* Actor : Actors)
	{
		if (Actor && !Actor->IsPendingDestroy())
		{
			Actor->Tick(DeltaTime);
		}
	}

	CleanupDestroyedActors();
}
