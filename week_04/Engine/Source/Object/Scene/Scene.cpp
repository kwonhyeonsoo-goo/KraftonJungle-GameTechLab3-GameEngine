#include "Scene.h"

#include "Core/Core.h"
#include "Core/Paths.h"

#include "Object/Actor/Actor.h"
#include "Object/Actor/CubeActor.h"
#include "Object/Actor/SphereActor.h"
#include "Camera/Camera.h"
#include "Component/PrimitiveComponent.h"
#include "Component/SphereComponent.h"
#include "Component/CubeComponent.h"
#include "Renderer/RenderCommand.h"
#include "Primitive/PrimitiveBase.h"
#include "Math/Frustum.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include "ThirdParty/nlohmann/json.hpp"
#include "Component/PrimitiveComponent.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/Material.h"

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

void UScene::InitializeDefaultScene(float AspectRatio, ID3D11Device* Device)
{
	// 카메라
	Camera = new CCamera();
	Camera->SetAspectRatio(AspectRatio);

	// JSON 파일에서 씬 로드 (카메라 포함)
	LoadSceneFromFile(FPaths::SceneDir() + "DefaultScene.json", Device);

	//Test
	AActor* Actor = SpawnActor<AActor>("TestActor");
	UPrimitiveComponent* SpehreComp = new USphereComponent();
	UPrimitiveComponent* CubeComp = new UCubeComponent();
	CubeComp->AttachTo(SpehreComp);
	CubeComp->SetRelativeTransform({ FRotator::MakeFromEuler({ 45.0f, 45.0f, 45.0f }), {0.0f, 0.0f, 2.0f}, {0.5f, 0.5f, 0.5f} });
	Actor->AddOwnedComponent(SpehreComp);
	Actor->AddOwnedComponent(CubeComp);
	Actor->SetActorLocation({ 0.0f, 0.0f, 12.0f });
}

void UScene::LoadSceneFromFile(const FString& FilePath, ID3D11Device* Device)
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

	// Material 에셋 사전 로드 (프로젝트 루트 기준 상대 경로)
	if (Device && Json.contains("Materials"))
	{
		for (auto& MatPath : Json["Materials"])
		{
			FString RelPath = MatPath.get<FString>();
			FString AbsPath = FPaths::Combine(FPaths::ProjectRoot(), RelPath);
			FMaterialManager::Get().GetOrLoad(Device, AbsPath);
		}
	}

	if (!Json.contains("Primitives")) return;

	int32 ActorIndex = 0;
	for (auto& [Key, Value] : Json["Primitives"].items())
	{
		FString Type = Value.value("Type", "");
		FString ActorName = Type + "_" + std::to_string(ActorIndex);

		AActor* Actor = nullptr;
		if (Type == "Sphere")
		{
			Actor = SpawnActor<ASphereActor>(ActorName);
		}
		else if (Type == "Cube")
		{
			Actor = SpawnActor<ACubeActor>(ActorName);
		}
		else
		{
			++ActorIndex;
			continue;
		}

		// Material 적용
		if (Value.contains("Material"))
		{
			FString MatName = Value["Material"].get<FString>();
			auto Mat = FMaterialManager::Get().FindByName(MatName);
			if (Mat)
			{
				UPrimitiveComponent* PrimComp = Actor->GetComponentByClass<UPrimitiveComponent>();
				if (PrimComp)
				{
					PrimComp->SetMaterial(Mat.get());
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
	int32 Index = 0;
	for (AActor* Actor : Actors)
	{
		if (!Actor || Actor->IsPendingDestroy())
			continue;

		FString Type;
		if (Actor->IsA(ASphereActor::StaticClass()))
			Type = "Sphere";
		else if (Actor->IsA(ACubeActor::StaticClass()))
			Type = "Cube";
		else
			continue;

		FTransform Transform = Actor->GetRootComponent()->GetRelativeTransform();
		const FVector EulerDegrees = Transform.Rotator().Euler();
		FString Key = std::to_string(Index);

		Primitives[Key]["Type"] = Type;

		// Material 이름 저장
		UPrimitiveComponent* PrimComp = Actor->GetComponentByClass<UPrimitiveComponent>();
		if (PrimComp && PrimComp->GetMaterial() && !PrimComp->GetMaterial()->GetName().empty())
		{
			Primitives[Key]["Material"] = PrimComp->GetMaterial()->GetName();
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

void UScene::CullVisiblePrimitives(const FFrustum& Frustum, TArray<UPrimitiveComponent*>& OutVisible)
{
	for (AActor* Actor : Actors)
	{
		if (!Actor || Actor->IsPendingDestroy())
		{
			continue;
		}

		for (UActorComponent* Comp : Actor->GetComponents())
		{
			if (!Comp->IsA(UPrimitiveComponent::StaticClass()))
			{
				continue;
			}
			UPrimitiveComponent* PrimComp = static_cast<UPrimitiveComponent*>(Comp);
			if (!PrimComp->GetPrimitive() || !PrimComp->GetPrimitive()->GetMeshData())
			{
				continue;
			}

			if (Frustum.IsVisible(PrimComp->GetWorldBounds()))
			{
				OutVisible.push_back(PrimComp);
			}
		}
	}
}

void UScene::CollectRenderCommands(const FFrustum& Frustum, FRenderCommandQueue& OutQueue)
{
	// 1단계: 컬링 — 가시 PrimitiveComponent 수집
	TArray<UPrimitiveComponent*> VisiblePrimitives;
	CullVisiblePrimitives(Frustum, VisiblePrimitives);

	// 2단계: 커맨드 수집 — RenderCommand 생성 (GPU 리소스 접근 없음)
	// SortKey는 Renderer::AddCommand에서 DefaultMaterial 폴백 포함하여 계산
	for (UPrimitiveComponent* PrimComp : VisiblePrimitives)
	{
		FRenderCommand Cmd;
		Cmd.MeshData = PrimComp->GetPrimitive()->GetMeshData();
		Cmd.WorldMatrix = PrimComp->GetWorldTransform();
		Cmd.Material = PrimComp->GetMaterial();

		OutQueue.AddCommand(Cmd);
	}
}
