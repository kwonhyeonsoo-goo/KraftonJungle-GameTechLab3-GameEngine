#include "Pawn.h"

#include "Controller.h"
#include "Component/CameraComponent.h"
#include "Component/BillboardComponent.h"
#include "Camera/Camera.h"
#include "Math/MathUtility.h"
#include "Object/Class.h"
#include "Object/ObjectFactory.h"
#include "Debug/EngineLog.h"
#include "Renderer/Renderer.h"

IMPLEMENT_RTTI(APawn, AActor)

APawn::~APawn()
{
}

void APawn::PostSpawnInitialize()
{
	AActor::PostSpawnInitialize();

	Camera = FObjectFactory::ConstructObject<UCameraComponent>(this, "Camera");
	AddOwnedComponent(Camera);

	if (GetComponentByClass<UBillboardComponent>() == nullptr)
	{
		UBillboardComponent* Billboard =
			FObjectFactory::ConstructObject<UBillboardComponent>(this, "BillboardComponent");

		if (Billboard)
		{
			Billboard->Initialize();
			AddOwnedComponent(Billboard);
			if (RootComponent && RootComponent != Billboard)
			{
				Billboard->AttachTo(RootComponent);
			}

			extern ENGINE_API class FRenderer* GRenderer;
			if (GRenderer)
			{
				Billboard->SetTexturePath(GRenderer->GetDevice(), "Editor/Icon/Pawn_64x.png");
			}
		}
	}
}

void APawn::BeginPlay()
{
	AActor::BeginPlay();

	if (Camera == nullptr)
	{
		Camera = GetComponentByClass<UCameraComponent>();
	}
	SetRootComponent(Camera);
}

void APawn::Tick(float DeltaTime)
{
	AActor::Tick(DeltaTime);

	if (Camera)
	{
		if (!MoveDirection.IsNearlyZero())
		{
			FVector NormalizedDir = MoveDirection.GetSafeNormal();
			FVector ForwardDirection = Camera->GetCamera()->GetForward().GetSafeNormal();
			FVector RightDirection = Camera->GetCamera()->GetRight().GetSafeNormal();

			FVector MovementOffset = ForwardDirection * NormalizedDir.Y + RightDirection * NormalizedDir.X;

			FVector NewLocation = GetActorLocation() + MovementOffset * 5.0f * DeltaTime;
			SetActorLocation(NewLocation);
		}


		FCamera* CameraTransform = Camera->GetCamera();
		if (CameraTransform)
		{
			float MouseX = GInput->GetMouseDeltaX();
			float MouseY = GInput->GetMouseDeltaY();

			const float NewYaw = CameraTransform->GetYaw() + MouseX * CameraTransform->GetMouseSensitivity();
			const float NewPitch = FMath::Clamp<float>(
				CameraTransform->GetPitch() - MouseY * CameraTransform->GetMouseSensitivity(),
				-89.0f,
				89.0f);
			CameraTransform->SetRotation(NewYaw, NewPitch);
		}
	}
}

void APawn::EndPlay()
{
	if (Controller)
	{
		UnPossessed();
	}

	AActor::EndPlay();
}

void APawn::ProcessInput(int32 KeyCode, EInputEventType EventType)
{
	float Value = 0.0f;
	if (EventType == EInputEventType::KeyDown)
	{
		Value = 1.0f;
	}
	else if (EventType == EInputEventType::KeyUp)
	{
		Value = 0.0f;
	}

	MoveDirection = FVector::Zero();

	switch (KeyCode)
	{
	case 'W': MoveDirection.Y = Value; break;
	case 'S': MoveDirection.Y = -Value; break;
	case 'D': MoveDirection.X = Value; break;
	case 'A': MoveDirection.X = -Value; break;
	}
}

void APawn::PossessedBy(AController* NewController)
{
	Controller = NewController;
	Controller->SetCamera(Camera);
}

void APawn::UnPossessed()
{
	Controller->SetCamera(nullptr);
	Controller = nullptr;
}

void APawn::DuplicateSubObjects()
{
	AActor::DuplicateSubObjects();
	Controller = nullptr;
	Camera = nullptr;
}
