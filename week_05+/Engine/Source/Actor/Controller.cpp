#include "Controller.h"

#include "Pawn.h"
#include "Component/CameraComponent.h"
#include "Object/Class.h"
#include "Input/EnhancedInputManager.h"
#include "Input/InputMappingContext.h"

IMPLEMENT_RTTI(AController, AActor)

AController::~AController()
{
	ClearPawnInput();
}

void AController::PostSpawnInitialize()
{
}

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

	ClearPawnInput();

	if (Pawn)
	{
		Pawn->UnPossessed();
		Pawn = nullptr;
	}
}

void AController::Possess(APawn* InPawn)
{
	if (!InPawn) return;

	if (Pawn)
	{
		ClearPawnInput();
		Pawn->UnPossessed();
	}

	Pawn = InPawn;
	Pawn->PossessedBy(this);

	// EnhancedInput이 이미 설정되어 있으면 즉시 Pawn 입력 바인딩
	if (EnhancedInput)
	{
		SetupPawnInput();
	}
}

void AController::SetupInput(FInputManager* InInputManager, FEnhancedInputManager* InEnhancedInput)
{
	// 이미 동일한 입력 시스템이 설정되어 있으면 스킵
	if (InputManager == InInputManager && EnhancedInput == InEnhancedInput)
		return;

	ClearPawnInput();

	InputManager = InInputManager;
	EnhancedInput = InEnhancedInput;

	// 이미 Possess된 Pawn이 있으면 입력 바인딩
	if (Pawn && EnhancedInput)
	{
		SetupPawnInput();
	}
}

void AController::SetupPawnInput()
{
	ClearPawnInput();

	if (!Pawn || !EnhancedInput) return;

	PawnInputContext = new FInputMappingContext();
	PawnInputContext->ContextName = "PawnInput";

	Pawn->SetupPlayerInputComponent(EnhancedInput, PawnInputContext);
	EnhancedInput->AddMappingContext(PawnInputContext, 0);
}

void AController::ClearPawnInput()
{
	if (PawnInputContext && EnhancedInput)
	{
		EnhancedInput->RemoveMappingContext(PawnInputContext);
		EnhancedInput->ClearBindings();
	}
	delete PawnInputContext;
	PawnInputContext = nullptr;
}

void AController::DuplicateSubObjects()
{
	AActor::DuplicateSubObjects();
	Pawn = nullptr;
	PawnInputContext = nullptr;
	EnhancedInput = nullptr;
	InputManager = nullptr;
}
