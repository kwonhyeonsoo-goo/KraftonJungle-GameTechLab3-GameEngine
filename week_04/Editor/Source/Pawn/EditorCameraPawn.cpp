#include "EditorCameraPawn.h"
namespace
{
	UObject* CreateEditorCameraPawnInstance(UObject* InOuter, const FString& InName)
	{
		return new AEditorCameraPawn(AEditorCameraPawn::StaticClass(), InName, InOuter);
	}
}
UClass* AEditorCameraPawn::StaticClass()
{
	static UClass ClassInfo("AEditorCameraPawn", AActor::StaticClass(), &CreateEditorCameraPawnInstance);
	return &ClassInfo;
}

AEditorCameraPawn::AEditorCameraPawn(UClass* InClass, const FString& InName, UObject* InOuter)
	: AActor(InClass, InName, InOuter)
{
	CameraCompenent = new UCameraComponent();
	AddOwnedComponent(CameraCompenent);

}
