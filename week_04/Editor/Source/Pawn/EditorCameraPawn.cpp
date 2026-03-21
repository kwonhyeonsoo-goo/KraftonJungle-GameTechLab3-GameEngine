#include "EditorCameraPawn.h"
#include "Object/ObjectFactory.h"
#include "Component/CameraComponent.h"

IMPLEMENT_RTTI(AEditorCameraPawn, AActor)

void AEditorCameraPawn::Initialize()
{
	CameraCompenent = FObjectFactory::ConstructObject<UCameraComponent>(this);
	AddOwnedComponent(CameraCompenent);
}
