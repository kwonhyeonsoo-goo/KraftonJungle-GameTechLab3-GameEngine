#include "World.h"
#include "Object/Class.h"  
#include "Scene/Scene.h"
#include "Object/ObjectFactory.h"
#include "Component/CameraComponent.h"
#include "Camera/Camera.h"
#include "Serializer/SceneSerializer.h"
#include "Core/Paths.h"
#include "Actor/Actor.h"
IMPLEMENT_RTTI(UWorld, UObject)

UWorld::~UWorld()
{
	CleanupWorld();
}

void UWorld::InitializeWorld(float AspectRatio, ID3D11Device* Device)
{
	Scene = FObjectFactory::ConstructObject<UScene>(this, "PersistentLevel");
	if (!Scene)
	{
		return;
	}

	Scene->SetSceneType(WorldType);

	if (!SceneCameraComponent)
	{
		SceneCameraComponent = FObjectFactory::ConstructObject<UCameraComponent>(this, "SceneCamera");
	}
	if (!ActiveCameraComponent)
	{
		ActiveCameraComponent = SceneCameraComponent;
	}
	if (SceneCameraComponent->GetCamera())
	{
		SceneCameraComponent->GetCamera()->SetAspectRatio(AspectRatio);
	}

	if (Device)
	{
		FSceneSerializer::Load(Scene, (FPaths::SceneDir() / "DefaultScene.json").string(), Device);
	}
}

void UWorld::BeginPlay()
{
	if (Scene)
	{
		Scene->BeginPlay();
	}
}

void UWorld::Tick(float InDeltaTime)
{
	DeltaSeconds = InDeltaTime;
	WorldTime += InDeltaTime;

	if (Scene)
	{
		Scene->Tick(InDeltaTime);
	}
}

void UWorld::CleanupWorld()
{
	if (Scene)
	{
		Scene->ClearActors();
		Scene->MarkPendingKill();
		Scene = nullptr;
	}
	if (SceneCameraComponent)
	{
		SceneCameraComponent->MarkPendingKill();
	}
	if (ActiveCameraComponent == SceneCameraComponent)
	{
		ActiveCameraComponent = nullptr;
	}
	SceneCameraComponent = nullptr;
	WorldTime = 0.f;
	DeltaSeconds = 0.f;
}

void UWorld::DestroyActor(AActor* InActor)
{
	if (!InActor || !Scene) return;


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

	Scene->DestroyActor(InActor);
}

const TArray<AActor*>& UWorld::GetActors() const
{
	static TArray<AActor*> EmptyArray;
	if (Scene)
	{
		return Scene->GetActors();
	}
	return EmptyArray;
}

void UWorld::SetActiveCameraComponent(UCameraComponent* InCamera)
{
	ActiveCameraComponent = InCamera ? InCamera : SceneCameraComponent;
}

UCameraComponent* UWorld::GetActiveCameraComponent() const
{
	return ActiveCameraComponent ? ActiveCameraComponent.Get() : SceneCameraComponent;
}

CCamera* UWorld::GetCamera() const
{
	UCameraComponent* Cam = GetActiveCameraComponent();
	return Cam ? Cam->GetCamera() : nullptr;
}

