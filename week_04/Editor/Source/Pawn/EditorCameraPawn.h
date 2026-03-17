
#pragma once
#include "Object/Actor/Actor.h"
#include "Component/CameraComponent.h"

class AEditorCameraPawn : public AActor
{
	AEditorCameraPawn(UClass* InClass, const FString& InName, UObject* InOuter = nullptr);


	UCameraComponent* GetCameraComponent() const { return CameraCompenent; }

private:
	UCameraComponent* CameraCompenent = nullptr;

};