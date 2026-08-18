#include "Pawn.h"

#include "Controller.h"
#include "Component/CameraComponent.h"
#include "Component/BillboardComponent.h"
#include "Component/TextRenderComponent.h"
#include "Camera/Camera.h"
#include "Math/MathUtility.h"
#include "Object/Class.h"
#include "Object/ObjectFactory.h"
#include "Debug/EngineLog.h"
#include "Renderer/Renderer.h"
#include "Input/EnhancedInputManager.h"
#include "Input/InputMappingContext.h"
#include "Input/InputTrigger.h"
#include "Input/InputModifier.h"

IMPLEMENT_RTTI(APawn, AActor)

APawn::~APawn()
{
}

void APawn::PostSpawnInitialize()
{
	AActor::PostSpawnInitialize();

	Camera = FObjectFactory::ConstructObject<UCameraComponent>(this, "Camera");
	AddOwnedComponent(Camera);
	SetRootComponent(Camera);

	// TextComponent가 AActor::PostSpawnInitialize에서 root로 생성됐으므로 Camera에 re-attach
	if (UTextRenderComponent* TextComp = GetComponentByClass<UTextRenderComponent>())
	{
		TextComp->AttachTo(Camera);
	}

	if (GetComponentByClass<UBillboardComponent>() == nullptr)
	{
		UBillboardComponent* Billboard =
			FObjectFactory::ConstructObject<UBillboardComponent>(this, "BillboardComponent");

		if (Billboard)
		{
			AddOwnedComponent(Billboard);
			Billboard->AttachTo(Camera);

			extern ENGINE_API class FRenderer* GRenderer;
			if (GRenderer)
			{
				Billboard->SetTexturePath(GRenderer->GetDevice(), GetDefaultBillboardIconPath());
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

	// 매 프레임 InputManager에서 직접 키 상태를 폴링하여 MoveDirection 갱신
	// (이벤트 누락으로 인한 stuck 방지)
	if (GInput)
	{
		MoveDirection = FVector::Zero();
		if (GInput->IsKeyDown('W')) MoveDirection.Y += 1.0f;
		if (GInput->IsKeyDown('S')) MoveDirection.Y -= 1.0f;
		if (GInput->IsKeyDown('D')) MoveDirection.X += 1.0f;
		if (GInput->IsKeyDown('A')) MoveDirection.X -= 1.0f;
	}

	if (Camera)
	{
		// 이동
		if (!MoveDirection.IsNearlyZero())
		{
			FVector NormalizedDir = MoveDirection.GetSafeNormal();
			FVector ForwardDirection = Camera->GetCamera()->GetForward().GetSafeNormal();
			FVector RightDirection = Camera->GetCamera()->GetRight().GetSafeNormal();

			FVector MovementOffset = ForwardDirection * NormalizedDir.Y + RightDirection * NormalizedDir.X;

			FVector NewLocation = GetActorLocation() + MovementOffset * 5.0f * DeltaTime;
			SetActorLocation(NewLocation);
		}

		// 카메라 회전 (마우스 델타 직접 폴링)
		if (GInput && GInput->IsGameMode())
		{
			FCamera* Cam = Camera->GetCamera();
			if (Cam)
			{
				float MouseX = GInput->GetMouseDeltaX();
				float MouseY = GInput->GetMouseDeltaY();
				if (MouseX != 0.0f || MouseY != 0.0f)
				{
					float NewYaw = Cam->GetYaw() + MouseX * Cam->GetMouseSensitivity();
					float NewPitch = FMath::Clamp<float>(
						Cam->GetPitch() - MouseY * Cam->GetMouseSensitivity(),
						-89.0f, 89.0f);
					Cam->SetRotation(NewYaw, NewPitch);
				}
			}
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

void APawn::SetupPlayerInputComponent(FEnhancedInputManager* EnhancedInput, FInputMappingContext* InputContext)
{
	// 기본 APawn은 Tick에서 직접 폴링 (WASD 이동 + 마우스 카메라 회전)
	// 서브클래스에서 override하여 EnhancedInput 기반 커스텀 바인딩 가능
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

FString APawn::GetDefaultBillboardIconPath() const
{
	std::wstring AbsoluteWPath = (FPaths::EditorIconDir() / L"Pawn_64x.png").wstring();
	FString AbsolutePath = FPaths::ToString(AbsoluteWPath);
	return FPaths::ToRelativePath(AbsolutePath);
}
