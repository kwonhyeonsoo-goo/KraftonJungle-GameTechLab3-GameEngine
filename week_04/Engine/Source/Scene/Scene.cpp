#include "Scene.h"

#include "Core/Paths.h"
#include "Actor/Actor.h"
#include "Actor/AttachTestActor.h"
#include "Actor/CubeActor.h"
#include "Actor/SphereActor.h"
#include "Camera/Camera.h"
#include "Component/CameraComponent.h"
#include "Component/PrimitiveComponent.h"
#include "Math/Frustum.h"
#include "Primitive/PrimitiveBase.h"
#include "Renderer/Material.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/RenderCommand.h"
#include "ThirdParty/nlohmann/json.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>

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

	if (ActiveCameraComponent == SceneCameraComponent)
	{
		ActiveCameraComponent = nullptr;
	}

	delete SceneCameraComponent;
	SceneCameraComponent = nullptr;
}

void UScene::SetActiveCameraComponent(UCameraComponent* InCameraComponent)
{
	ActiveCameraComponent = InCameraComponent ? InCameraComponent : SceneCameraComponent;
}

UCameraComponent* UScene::GetActiveCameraComponent() const
{
	return ActiveCameraComponent ? ActiveCameraComponent : SceneCameraComponent;
}

CCamera* UScene::GetCamera() const
{
	UCameraComponent* CameraComponent = GetActiveCameraComponent();
	return CameraComponent ? CameraComponent->GetCamera() : nullptr;
}

void UScene::InitializeEmptyScene(float AspectRatio)
{
	if (SceneCameraComponent == nullptr)
	{
		SceneCameraComponent = new UCameraComponent();
	}

	if (ActiveCameraComponent == nullptr)
	{
		ActiveCameraComponent = SceneCameraComponent;
	}

	if (SceneCameraComponent->GetCamera())
	{
		SceneCameraComponent->GetCamera()->SetAspectRatio(AspectRatio);
	}

	if (ActiveCameraComponent != SceneCameraComponent && GetCamera())
	{
		GetCamera()->SetAspectRatio(AspectRatio);
	}
}

void UScene::InitializeDefaultScene(float AspectRatio, ID3D11Device* Device)
{
	InitializeEmptyScene(AspectRatio);
	LoadSceneFromFile(FPaths::SceneDir() + "DefaultScene.json", Device);
}

void UScene::LoadSceneFromFile(const FString& FilePath, ID3D11Device* Device)
{
	std::ifstream File(FilePath);
	if (!File.is_open())
	{
		return;
	}

	nlohmann::json Json;
	File >> Json;

	if (CCamera* Camera = GetCamera(); Camera && Json.contains("Camera"))
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
			const FString AbsolutePath = FPaths::Combine(FPaths::ProjectRoot(), RelativePath);
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
			Actor = SpawnActor<ASphereActor>(ActorName);
		}
		else if (Type == "Cube")
		{
			Actor = SpawnActor<ACubeActor>(ActorName);
		}
		else if (Type == "AttachTest")
		{
			Actor = SpawnActor<AAttachTestActor>(ActorName);
		}
		else
		{
			++ActorIndex;
			continue;
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

		++ActorIndex;
	}
}

void UScene::SaveSceneToFile(const FString& FilePath)
{
	nlohmann::json Json;

	if (CCamera* Camera = GetCamera())
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
		FString Root = FPaths::ProjectRoot();
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
	for (AActor* Actor : Actors)
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

		const FTransform Transform = Actor->GetRootComponent()->GetRelativeTransform();
		const FVector EulerDegrees = Transform.Rotator().Euler();
		const FString Key = std::to_string(Index);

		Primitives[Key]["Type"] = Type;

		// Material 이름 저장 (에셋 원본 이름 사용)
		UPrimitiveComponent* PrimComp = Actor->GetComponentByClass<UPrimitiveComponent>();
		if (PrimComp && PrimComp->GetMaterial() && !PrimComp->GetMaterial()->GetOriginName().empty())
		//if (UPrimitiveComponent* PrimitiveComponent = Actor->GetComponentByClass<UPrimitiveComponent>())
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

	std::ofstream File(FilePath);
	if (File.is_open())
	{
		File << std::setw(2) << Json << std::endl;
	}
}

void UScene::ClearActors()
{
	if (ActiveCameraComponent != SceneCameraComponent)
	{
		ActiveCameraComponent = SceneCameraComponent;
	}

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

	const auto It = std::find(Actors.begin(), Actors.end(), InActor);
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

	if (ActiveCameraComponent && ActiveCameraComponent != SceneCameraComponent)
	{
		for (UActorComponent* Component : InActor->GetComponents())
		{
			if (Component == ActiveCameraComponent)
			{
				ActiveCameraComponent = SceneCameraComponent;
				break;
			}
		}
	}

	InActor->Destroy();
}

void UScene::CleanupDestroyedActors()
{
	const auto NewEnd = std::ranges::remove_if(Actors,
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

void UScene::CullVisiblePrimitives(const FFrustum& Frustum, TArray<UPrimitiveComponent*>& OutVisible)
{
	for (AActor* Actor : Actors)
	{
		if (!Actor || Actor->IsPendingDestroy())
		{
			continue;
		}

		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (!Component->IsA(UPrimitiveComponent::StaticClass()))
			{
				continue;
			}

			UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
			if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData())
			{
				continue;
			}

			if (Frustum.IsVisible(PrimitiveComponent->GetWorldBounds()))
			{
				OutVisible.push_back(PrimitiveComponent);
			}
		}
	}
}

void UScene::CollectRenderCommands(const FFrustum& Frustum, FRenderCommandQueue& OutQueue)
{
	TArray<UPrimitiveComponent*> VisiblePrimitives;
	CullVisiblePrimitives(Frustum, VisiblePrimitives);

	for (UPrimitiveComponent* PrimitiveComponent : VisiblePrimitives)
	{
		FRenderCommand Command;
		Command.MeshData = PrimitiveComponent->GetPrimitive()->GetMeshData();
		Command.WorldMatrix = PrimitiveComponent->GetWorldTransform();
		Command.Material = PrimitiveComponent->GetMaterial();
		OutQueue.AddCommand(Command);
	}
}
