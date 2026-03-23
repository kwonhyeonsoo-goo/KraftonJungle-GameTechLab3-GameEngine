#include "SubUVActor.h"
#include "Component/SubUVComponent.h"
#include "Object/ObjectFactory.h"
#include "Object/Class.h"

IMPLEMENT_RTTI(ASubUVActor, AActor)

void ASubUVActor::PostSpawnInitialize()
{
	SubUVComponent = FObjectFactory::ConstructObject<USubUVComponent>(this, "SubUVComponent");
	AddOwnedComponent(SubUVComponent);

	AActor::PostSpawnInitialize();
}