#include "EditorCameraPawn.h"

AEditorCameraPawn::AEditorCameraPawn(UClass* InClass, const FString& InName, UObject* InOuter)
	: AActor(InClass, InName, InOuter)
{
	CameraCompenent = new UCameraComponent();
	AddOwnedComponent(CameraCompenent);

}
