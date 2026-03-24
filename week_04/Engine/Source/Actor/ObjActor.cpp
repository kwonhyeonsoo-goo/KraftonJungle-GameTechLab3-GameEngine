#include "ObjActor.h"
#include "Component/ObjComponent.h"
#include "Component/RandomColorComponent.h"
#include "Object/ObjectFactory.h"
#include "Object/Class.h"

IMPLEMENT_RTTI(AObjActor, AActor)

void AObjActor::LoadObj(const FString& FilePath)
{
	if (ObjComponent)
		ObjComponent->LoadPrimitive(FilePath);
}

void AObjActor::PostSpawnInitialize()
{
	ObjComponent = FObjectFactory::ConstructObject<UObjComponent>(this);
	PrimitiveComponent = ObjComponent;
	AddOwnedComponent(PrimitiveComponent);

	if (bUseRandomColor)
	{
		RandomColorComponent = FObjectFactory::ConstructObject<URandomColorComponent>(this);
		AddOwnedComponent(RandomColorComponent);
	}

	AActor::PostSpawnInitialize();
}
