#include "World.h"
#include "Object/Class.h"  
#include "Scene/Scene.h"
#include "Object/ObjectFactory.h"
#include "Component/CameraComponent.h"
#include "Camera/Camera.h"

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

	if (Device)
	{
		Scene->InitializeDefaultScene(AspectRatio, Device);
	}
	else
	{
		Scene->InitializeEmptyScene(AspectRatio);
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

	WorldTime = 0.f;
	DeltaSeconds = 0.f;
}

void UWorld::DestroyActor(AActor* InActor)
{
	if (Scene)
	{
		Scene->DestroyActor(InActor);
	}
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
	if (Scene)
	{
		Scene->SetActiveCameraComponent(InCamera);
	}
}

UCameraComponent* UWorld::GetActiveCameraComponent() const
{
	return Scene ? Scene->GetActiveCameraComponent() : nullptr;
}

CCamera* UWorld::GetCamera() const
{
	return Scene ? Scene->GetCamera() : nullptr;
}

