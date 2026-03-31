#include "CameraPawn.h"
#include "Object/ObjectFactory.h"
#include "Object/Class.h"
#include "Component/CameraComponent.h"

IMPLEMENT_RTTI(UCameraPawn, AActor)

void UCameraPawn::Initialize()
{
	CameraCompenent = FObjectFactory::ConstructObject<UCameraComponent>(this);
	AddOwnedComponent(CameraCompenent);
}
