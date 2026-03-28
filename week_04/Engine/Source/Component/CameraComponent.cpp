#include "CameraComponent.h"
#include "Object/Class.h"
#include "Camera/Camera.h"

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
	Camera->SetPosition(Camera->GetPosition() + Right * (Value * Camera->GetSpeed()));
}

void UCameraComponent::PanUp(float Value)
{
	//카메라 기준 local up을 계산합니다.
	const FVector Forward = Camera->GetForward().GetSafeNormal();
	const FVector Right = Camera->GetRight().GetSafeNormal();
	const FVector PanUp = FVector::CrossProduct(Forward, Right).GetSafeNormal();
	Camera->SetPosition(Camera->GetPosition() + PanUp * (Value * Camera->GetSpeed()));
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
