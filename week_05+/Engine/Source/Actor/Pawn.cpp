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

	if (Camera && !MoveDirection.IsNearlyZero())
	{
		FVector NormalizedDir = MoveDirection.GetSafeNormal();
		FVector ForwardDirection = Camera->GetCamera()->GetForward().GetSafeNormal();
		FVector RightDirection = Camera->GetCamera()->GetRight().GetSafeNormal();

		FVector MovementOffset = ForwardDirection * NormalizedDir.Y + RightDirection * NormalizedDir.X;

		FVector NewLocation = GetActorLocation() + MovementOffset * 5.0f * DeltaTime;
		SetActorLocation(NewLocation);
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
	// ─── 이동: WASD → Axis2D (X=좌우, Y=전후) ───
	{
		auto& Forward = InputContext->AddMapping(&MoveAction, 'W');
		Forward.Triggers.push_back(new FTriggerDown());
		auto* Swizzle = new FModifierSwizzleAxis();
		Swizzle->Order = FModifierSwizzleAxis::ESwizzleOrder::YXZ; // X값 → Y로
		Forward.Modifiers.push_back(Swizzle);
	}
	{
		auto& Backward = InputContext->AddMapping(&MoveAction, 'S');
		Backward.Triggers.push_back(new FTriggerDown());
		auto* Swizzle = new FModifierSwizzleAxis();
		Swizzle->Order = FModifierSwizzleAxis::ESwizzleOrder::YXZ;
		Backward.Modifiers.push_back(Swizzle);
		Backward.Modifiers.push_back(new FModifierNegative());
	}
	{
		auto& Right = InputContext->AddMapping(&MoveAction, 'D');
		Right.Triggers.push_back(new FTriggerDown());
	}
	{
		auto& Left = InputContext->AddMapping(&MoveAction, 'A');
		Left.Triggers.push_back(new FTriggerDown());
		Left.Modifiers.push_back(new FModifierNegative());
	}

	// ─── 시야: Mouse Delta → Axis2D (X=Yaw, Y=Pitch) ───
	{
		auto& LookX = InputContext->AddMapping(&LookAction, static_cast<int32>(EInputKey::MouseX));
		LookX.Triggers.push_back(new FTriggerDown());
	}
	{
		auto& LookY = InputContext->AddMapping(&LookAction, static_cast<int32>(EInputKey::MouseY));
		LookY.Triggers.push_back(new FTriggerDown());
		auto* Swizzle = new FModifierSwizzleAxis();
		Swizzle->Order = FModifierSwizzleAxis::ESwizzleOrder::YXZ; // X값 → Y로
		LookY.Modifiers.push_back(Swizzle);
	}

	// ─── 콜백 바인딩 ───
	EnhancedInput->BindAction(&MoveAction, ETriggerEvent::Triggered,
		[this](const FInputActionValue& Value) {
			MoveDirection = Value.GetVector();
		});

	EnhancedInput->BindAction(&MoveAction, ETriggerEvent::Completed,
		[this](const FInputActionValue&) {
			MoveDirection = FVector::Zero();
		});

	EnhancedInput->BindAction(&LookAction, ETriggerEvent::Triggered,
		[this](const FInputActionValue& Value) {
			if (Camera)
			{
				FCamera* Cam = Camera->GetCamera();
				if (Cam)
				{
					FVector Look = Value.GetVector();
					float NewYaw = Cam->GetYaw() + Look.X * Cam->GetMouseSensitivity();
					float NewPitch = FMath::Clamp<float>(
						Cam->GetPitch() - Look.Y * Cam->GetMouseSensitivity(),
						-89.0f, 89.0f);
					Cam->SetRotation(NewYaw, NewPitch);
				}
			}
		});
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
