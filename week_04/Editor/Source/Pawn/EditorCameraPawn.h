
#pragma once
#include "Actor/Actor.h"
#include "Component/CameraComponent.h"

class AEditorCameraPawn : public AActor
{
public:
	static UClass* StaticClass();

	AEditorCameraPawn(UClass* InClass, const FString& InName, UObject* InOuter = nullptr);
	UCameraComponent* GetCameraComponent() const { return CameraCompenent; }

private:
	UCameraComponent* CameraCompenent = nullptr;

};
