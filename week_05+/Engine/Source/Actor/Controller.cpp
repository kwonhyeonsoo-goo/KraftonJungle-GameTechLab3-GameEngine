#include "Controller.h"

#include "Pawn.h"
#include "Component/CameraComponent.h"
#include "Object/Class.h"

IMPLEMENT_RTTI(AController, AActor)

void AController::BeginPlay()
{
	AActor::BeginPlay();

	APawn* DefaultPawn = GWorld->GetDefaultPawn();
	if (DefaultPawn)
		Possess(DefaultPawn);
}

void AController::Tick(float DeltaTime)
{
	AActor::Tick(DeltaTime);
}

void AController::EndPlay()
{
	AActor::EndPlay();

	if (Pawn)
	{
		Pawn->UnPossessed();
		Pawn = nullptr;
	}
}

void AController::ProcessInput(int32 KeyCode, EInputEventType EventType)
{
	if (Pawn)
	{
		Pawn->ProcessInput(KeyCode, EventType);
	}
}

void AController::Possess(APawn* InPawn)
{
	if (InPawn)
	{
		if (Pawn)
			Pawn->UnPossessed();

		Pawn = InPawn;

		if (Pawn)
			Pawn->PossessedBy(this);
	}
}

void AController::DuplicateSubObjects()
{
	AActor::DuplicateSubObjects();
	Pawn = nullptr;
}
