#include "StaticMeshActor.h"
#include "Object/Class.h"
#include "Component/StaticMeshComponent.h"
#include "Object/ObjectFactory.h"
#include "Object/Mesh/StaticMesh.h"

IMPLEMENT_RTTI(AStaticMeshActor, AActor)

void AStaticMeshActor::PostSpawnInitialize()
{
	StaticMeshComponent = FObjectFactory::ConstructObject<UStaticMeshComponent>(this);
	AddOwnedComponent(StaticMeshComponent);

	AActor::PostSpawnInitialize();
}

void AStaticMeshActor::SetStaticMesh(UStaticMesh* InStaticMesh)
{
	StaticMeshComponent->SetStaticMesh(InStaticMesh);
}

FStaticMesh* AStaticMeshActor::GetStaticMeshAsset() const
{
	if (StaticMeshComponent && StaticMeshComponent->GetStaticMesh())
	{
		return StaticMeshComponent->GetStaticMesh()->GetAsset();
	}

	return nullptr;
}