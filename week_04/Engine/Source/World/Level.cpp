#include "Level.h"
#include "Core/Paths.h"
#include "Actor/Actor.h"
#include "Actor/AttachTestActor.h"
#include "Actor/CubeActor.h"
#include "Actor/SphereActor.h"
#include "Actor/SubUVActor.h"
#include "Actor/CameraPawn.h"
#include "Component/CameraComponent.h"
#include "Object/ObjectFactory.h"
#include "Component/PrimitiveComponent.h"
#include "Object/Class.h"
#include "Serializer/SceneSerializer.h"
#include "Component/LineBatchComponent.h"
#include <algorithm>

IMPLEMENT_RTTI(ULevel, UObject)

ULevel::~ULevel()
{
	for (AActor* Actor : Actors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
	Actors.clear();
}

void ULevel::ClearActors()
{
	for (AActor* Actor : Actors)
	{
		if (Actor->IsA<UCameraPawn>())
		{
			continue;
		}
		if (Actor)
		{
			Actor->Destroy();
		}
	}
	Actors.clear();
	bBegunPlay = false;
}

void ULevel::RegisterActor(AActor* InActor)
{
	if (!InActor) return;

	const auto It = std::find(Actors.begin(), Actors.end(), InActor);
	if (It != Actors.end()) return;

	Actors.push_back(InActor);
	InActor->SetLevel(this);
}

void ULevel::DestroyActor(AActor* InActor)
{
	if (!InActor) return;
	InActor->Destroy();
}

void ULevel::CleanupDestroyedActors()
{
	const auto NewEnd = std::ranges::remove_if(Actors,
		[](const AActor* Actor)
		{
			return Actor == nullptr || Actor->IsPendingDestroy();
		}).begin();

	Actors.erase(NewEnd, Actors.end());
}

void ULevel::BeginPlay()
{
	if (bBegunPlay) return;
	bBegunPlay = true;

	for (AActor* Actor : Actors)
	{
		if (Actor && !Actor->HasBegunPlay())
		{
			Actor->BeginPlay();
		}
	}
}

void ULevel::Tick(float DeltaTime)
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