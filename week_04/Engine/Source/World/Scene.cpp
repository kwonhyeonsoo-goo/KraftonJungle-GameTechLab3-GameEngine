#include "Scene.h"
#include "Component/PrimitiveComponent.h"
#include "Component/StaticMeshComponent.h"

void FScene::AddComponent(UPrimitiveComponent* Comp)
{
	Primitives.push_back(Comp);
}

void FScene::Clear()
{
	Primitives.clear();
}

const TArray<UStaticMeshComponent*> FScene::GetStaticMeshComponents() const
{
	TArray<UStaticMeshComponent*> StaticMeshComps;

	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (Primitive->IsA(UStaticMeshComponent::StaticClass()))
			StaticMeshComps.push_back(static_cast<UStaticMeshComponent*>(Primitive));
	}

	return StaticMeshComps;
}
