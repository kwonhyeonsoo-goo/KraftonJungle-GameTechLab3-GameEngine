#include "CameraComponent.h"
#include "Object/Class.h"
#include "Camera/Camera.h"
#include "Serializer/Archive.h"
IMPLEMENT_RTTI(UCameraComponent, USceneComponent)

void UCameraComponent::Initialize()
{
	bCanEverTick = true;
	Camera = new FCamera();
}

UCameraComponent::~UCameraComponent()
{
	delete Camera;
	Camera = nullptr;
}

void UCameraComponent::Tick(float DeltaTime)
{
	USceneComponent::Tick(DeltaTime);

	//TODO : will be add CameraArm, shake and interpolation  
}

void UCameraComponent::MoveForward(float Value)
{
	Camera->MoveForward(Value);
}

void UCameraComponent::MoveRight(float Value)
{
	Camera->MoveRight(Value);
}

void UCameraComponent::MoveUp(float Value)
{
	Camera->MoveUp(Value);
}

void UCameraComponent::PanRight(float Value)
{
	const FVector Right = Camera->GetRight().GetSafeNormal();
	Camera->OffsetPosition(Right * (Value * Camera->GetSpeed()));
}

void UCameraComponent::PanUp(float Value)
{
	//카메라 기준 local up을 계산합니다.
	const FVector Forward = Camera->GetForward().GetSafeNormal();
	const FVector Right = Camera->GetRight().GetSafeNormal();
	const FVector PanUp = FVector::CrossProduct(Forward, Right).GetSafeNormal();
	Camera->OffsetPosition(PanUp * (Value * Camera->GetSpeed()));
}

void UCameraComponent::FootZoom(float Value)
{
	if (Camera->IsOrthographic())
	{
		const float ZoomScale = (Value * 0.1f);
		Camera->SetOrthoWidth(Camera->GetOrthoWidth() * ZoomScale);
		return;
	}
#if IS_OBJ_VIEWER //뷰어에서는 발줌이 느립니다.
	Camera->MoveForward(Value*0.1f);
#else
	Camera->MoveForward(Value);
#endif
}

void UCameraComponent::Rotate(float DeltaYaw, float DeltaPitch)
{
	Camera->Rotate(DeltaYaw, DeltaPitch);
}

FCamera* UCameraComponent::GetCamera() const
{
	return Camera;
}

FMatrix UCameraComponent::GetViewMatrix() const
{
	return Camera->GetViewMatrix();
}

FMatrix UCameraComponent::GetProjectionMatrix() const
{
	return Camera->GetProjectionMatrix();
}

void UCameraComponent::SetFov(float inFov)
{
	Camera->SetFOV(inFov);
}

void UCameraComponent::SetSpeed(float Inspeed)
{
	Camera->SetSpeed(Inspeed);
}

void UCameraComponent::SetSensitivity(float InSetSensitivity)
{
	Camera->SetMouseSensitivity(InSetSensitivity);
}


void UCameraComponent::Serialize(FArchive& Ar)
{
	USceneComponent::Serialize(Ar);

	if (Ar.IsSaving())
	{
		float FOV = Camera->GetFOV();
		float Speed = Camera->GetSpeed();
		Ar.Serialize("FOV", FOV);
		Ar.Serialize("Speed", Speed);
	}
	else
	{
		if (Ar.Contains("FOV")) { float F; Ar.Serialize("FOV", F); Camera->SetFOV(F); }
		if (Ar.Contains("Speed")) { float S; Ar.Serialize("Speed", S); Camera->SetSpeed(S); }
	}
}