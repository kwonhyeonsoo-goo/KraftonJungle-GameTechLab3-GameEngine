#include "World.h"
#include "Object/Class.h"  
#include "World/Level.h"
#include "Object/ObjectFactory.h"
#include "Component/CameraComponent.h"
#include "Camera/Camera.h"
#include "Serializer/SceneSerializer.h"
#include "Core/Paths.h"
#include "Actor/Actor.h"
#include "Actor/Pawn.h"
#include "Actor/Controller.h"

IMPLEMENT_RTTI(UWorld, UObject)

UWorld* GWorld = nullptr;

UWorld::~UWorld()
{
	CleanupWorld();
}

void UWorld::InitializeWorld()
{
	Level = FObjectFactory::ConstructObject<ULevel>(this, "Level");
	if (!Level)
	{
		return;
	}

	Level->SetLevelType(WorldType);

	if (!LevelCameraComponent)
	{
		LevelCameraComponent = FObjectFactory::ConstructObject<UCameraComponent>(this, "LevelCamera");
	}
	if (!ActiveCameraComponent)
	{
		ActiveCameraComponent = LevelCameraComponent;
	}
}

void UWorld::BeginPlay()
{
	if (bBegunPlay) return;  
	bBegunPlay = true;

	DefaultPawn = nullptr;
	DefaultController = nullptr;

	// 현재는 Defualt Pawn과 Controller를 처음 발견된 객체로 지정합니다. TODO: 수정 필요
	for (AActor* Actor : GetActors())
	{
		if (APawn* Pawn = dynamic_cast<APawn*>(Actor))
		{
			DefaultPawn = Pawn;
		}

		if (AController* Controller = dynamic_cast<AController*>(Actor))
		{
			DefaultController = Controller;
		}
	}

	if (DefaultPawn == nullptr)
	{
		DefaultPawn = Level->SpawnActor<APawn>("DefaultPawn");
	}
	if (DefaultController == nullptr)
	{
		DefaultController = Level->SpawnActor<AController>("DefaultController");
	}

	if (Level)
	{
		Level->BeginPlay();
	}
}

void UWorld::Tick(float InDeltaTime)
{
	DeltaSeconds = InDeltaTime;
	WorldTime += InDeltaTime;

	if (Level)
	{
		Level->Tick(InDeltaTime);
	}
}

void UWorld::CleanupWorld()
{
	if (Level)
	{
		Level->ClearActors();
		Level->MarkPendingKill();
	}
	if (LevelCameraComponent)
	{
		LevelCameraComponent->MarkPendingKill();
	}
	if (ActiveCameraComponent == LevelCameraComponent)
	{
		ActiveCameraComponent = nullptr;
	}
	LevelCameraComponent = nullptr;
	WorldTime = 0.f;
	DeltaSeconds = 0.f;
}

void UWorld::DestroyActor(AActor* InActor)
{
	if (!InActor || !Level) return;

	if (ActiveCameraComponent && ActiveCameraComponent != LevelCameraComponent)
	{
		for (UActorComponent* Component : InActor->GetComponents())
		{
			if (Component == ActiveCameraComponent)
			{
				ActiveCameraComponent = LevelCameraComponent;
				break;
			}
		}
	}

	Level->DestroyActor(InActor);
}

TArray<AActor*> UWorld::GetActors() const
{
	TArray<AActor*> AllActors;
	if (Level)
	{
		const auto& LevelActors = Level->GetActors();
		AllActors.insert(AllActors.end(), LevelActors.begin(), LevelActors.end());
	}

	return AllActors;
}

void UWorld::SetActiveCameraComponent(UCameraComponent* InCamera)
{
	ActiveCameraComponent = InCamera ? InCamera : LevelCameraComponent;
}

UCameraComponent* UWorld::GetActiveCameraComponent() const
{
	return ActiveCameraComponent ? ActiveCameraComponent.Get() : LevelCameraComponent;
}

FCamera* UWorld::GetCamera() const
{
	UCameraComponent* Cam = GetActiveCameraComponent();
	return Cam ? Cam->GetCamera() : nullptr;
}

UWorld* UWorld::DuplicateWorldForPIE(UWorld* SourceWorld)
{
	if (!SourceWorld) return nullptr;

	UWorld* NewWorld = FObjectFactory::ConstructObject<UWorld>(nullptr, SourceWorld->GetName() + "_PIE");
	if (!NewWorld) return nullptr;

	NewWorld->SetWorldType(EWorldType::PIE);
	NewWorld->InitializeWorld();

	// Level 복제
	if (SourceWorld->GetLevel())
	{
		ULevel* NewLevel = static_cast<ULevel*>(SourceWorld->GetLevel()->Duplicate());
		if (NewLevel)
		{
			NewLevel->SetLevelType(EWorldType::PIE);
			NewWorld->Level = NewLevel;
		}
	}

	return NewWorld;
}
