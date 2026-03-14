#include "Scene.h"

#include "Core/Core.h"
#include "Object/Class.h"
#include "Object/Actor/Actor.h"
#include "Camera/Camera.h"
#include "Component/SphereComponent.h"
#include "Component/CubeComponent.h"
#include <algorithm>

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
	Camera->SetPosition({ -5.0f, 0.0f, 2.0f });
	Camera->SetRotation(0.0f, -15.0f);
	Camera->SetAspectRatio(AspectRatio);

	// Sphere Actor
	AActor* SphereActor = SpawnActor<AActor>("SphereActor");
	USphereComponent* SphereComp = new USphereComponent();
	SphereActor->AddOwnedComponent(SphereComp);
	SphereActor->SetActorLocation({ 0.0f, 0.0f, 0.0f });

	// Cube Actor
	AActor* CubeActor = SpawnActor<AActor>("CubeActor");
	UCubeComponent* CubeComp = new UCubeComponent();
	CubeActor->AddOwnedComponent(CubeComp);
	CubeActor->SetActorLocation({ 3.0f, 0.0f, 0.0f });
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
