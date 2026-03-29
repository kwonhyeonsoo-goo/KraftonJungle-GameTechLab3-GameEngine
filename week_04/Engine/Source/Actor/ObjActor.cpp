#include "ObjActor.h"
#include "Component/ObjComponent.h"
#include "Component/RandomColorComponent.h"
#include "Object/ObjectFactory.h"
#include "Object/Class.h"
#include <d3d11.h>


IMPLEMENT_RTTI(AObjActor, AActor)

void AObjActor::LoadObj(ID3D11Device* Device, const FString& FilePath)
{
	if (!Device) return;

	if (ObjComponent)
	{
		ObjComponent->LoadStaticMeshAsset(Device, FilePath);
	}
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
