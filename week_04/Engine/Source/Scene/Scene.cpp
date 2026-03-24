#include "Scene.h"

#include "Core/Paths.h"
#include "Actor/Actor.h"
#include "Actor/AttachTestActor.h"
#include "Actor/CubeActor.h"
#include "Actor/SphereActor.h"
#include "Actor/SubUVActor.h"
#include "Camera/Camera.h"
#include "Component/CameraComponent.h"
#include "Object/ObjectFactory.h"
#include "Component/PrimitiveComponent.h"
#include "Math/Frustum.h"
#include "Primitive/PrimitiveBase.h"
#include "Renderer/Material.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/RenderCommand.h"

#include "Component/UUIDBillboardComponent.h"
#include "Object/Class.h"
#include "Core/FEngine.h"
#include "Component/SubUVComponent.h"

#include "Serializer/SceneSerializer.h"
#include <algorithm>



#include "Component/LineBatchComponent.h"

IMPLEMENT_RTTI(UScene, UObject)

UScene::~UScene()
{
	for (AActor* Actor : Actors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
	Actors.clear();

	if (SceneCameraComponent)
	{
		SceneCameraComponent->MarkPendingKill();
	}

	if (ActiveCameraComponent == SceneCameraComponent)
	{
		ActiveCameraComponent = nullptr;
	}
	SceneCameraComponent = nullptr;
}

void UScene::SetActiveCameraComponent(UCameraComponent* InCameraComponent)
{
	ActiveCameraComponent = InCameraComponent ? InCameraComponent : SceneCameraComponent;
}

UCameraComponent* UScene::GetActiveCameraComponent() const
{
	return ActiveCameraComponent ? ActiveCameraComponent.Get() : SceneCameraComponent;
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
		SceneCameraComponent = FObjectFactory::ConstructObject<UCameraComponent>(this, "SceneCamera");
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
	//LoadSceneFromFile((FPaths::SceneDir() / "DefaultScene.json").string(), Device);
	FSceneSerializer::Load(this, (FPaths::SceneDir() / "DefaultScene.json").string(), Device);
}

void UScene::ClearActors()
{
	if (ActiveCameraComponent != SceneCameraComponent)
	{
		ActiveCameraComponent = SceneCameraComponent;
	}

	for (AActor* Actor : Actors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
	Actors.clear();

	bBegunPlay = false;
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

