#include "CameraComponent.h"
#include "Object/Class.h"

namespace
{
	UObject* CreateUCameraComponentInstance(UObject* InOuter, const FString& InName)
	{
		return new UCameraComponent(UCameraComponent::StaticClass(), InName, InOuter);
	}
}

UClass* UCameraComponent::StaticClass()
{
	static UClass ClassInfo("UCameraComponent", USceneComponent::StaticClass(), &CreateUCameraComponentInstance);
	return &ClassInfo;
}

UCameraComponent::UCameraComponent()
	: USceneComponent(StaticClass(), "")
{
	bCanEverTick = true;
	Camera = new CCamera();
}

UCameraComponent::UCameraComponent(UClass* InClass, const FString& InName, UObject* InOuter)
	: USceneComponent(InClass, InName, InOuter)
{
	bCanEverTick = true;
	Camera = new CCamera();
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

void UCameraComponent::Rotate(float DeltaYaw, float DeltaPitch)
{
	Camera->Rotate(DeltaYaw, DeltaPitch);
}

CCamera* UCameraComponent::GetCamera() const
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
