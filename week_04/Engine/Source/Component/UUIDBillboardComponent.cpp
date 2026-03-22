#include "UUIDBillboardComponent.h"
#include "Actor/Actor.h"
#include "Object/Class.h"


IMPLEMENT_RTTI(UUUIDBillboardComponent, UPrimitiveComponent)

void UUUIDBillboardComponent::Initialize()
{

}

FString UUUIDBillboardComponent::GetDisplayText() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return "";
	}

	return FString("UUID: ") + OwnerActor->GetUUIDString();
}

// 오브젝트 위쪽 위치 고려
FVector UUUIDBillboardComponent::GetTextWorldPosition() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return WorldOffset;
	}

	USceneComponent* Root = OwnerActor->GetRootComponent();
	if (!Root)
	{
		return WorldOffset;
	}

	const FVector RootLocation = Root->GetWorldLocation();

	bool bFoundPrimitiveBounds = false;
	float MaxTopZ = -std::numeric_limits<float>::infinity();

	for (UActorComponent* Component : OwnerActor->GetComponents())
	{
		if (!Component || !Component->IsA(UPrimitiveComponent::StaticClass()))
		{
			continue;
		}

		UPrimitiveComponent* PrimitiveComp = static_cast<UPrimitiveComponent*>(Component);

		if (PrimitiveComp == this)
		{
			continue;
		}

		const FBoxSphereBounds Bounds = PrimitiveComp->GetWorldBoundsForAABB();
		const float TopZ = Bounds.Center.Z + Bounds.BoxExtent.Z;

		if (!bFoundPrimitiveBounds || TopZ > MaxTopZ)
		{
			MaxTopZ = TopZ;
			bFoundPrimitiveBounds = true;
		}
	}

	if (bFoundPrimitiveBounds)
	{
		return FVector(
			RootLocation.X + WorldOffset.X,
			RootLocation.Y + WorldOffset.Y,
			MaxTopZ + WorldOffset.Z
		);
	}

	return RootLocation + WorldOffset;
}

FBoundingSphere UUUIDBillboardComponent::GetWorldBounds() const
{
	const FVector Center = GetTextWorldPosition();

	return { Center, BillboardScale * 3.0f };
}

FBoxSphereBounds UUUIDBillboardComponent::GetWorldBoundsForAABB() const
{
	const FVector Center = GetTextWorldPosition();
	// radius 를 정사각형 extent 에 맞게 줄이기
	const FVector Extent(BillboardScale * 3.0f * 0.707f, BillboardScale * 3.0f * 0.707f, BillboardScale * 3.0f * 0.707f);

	return { Center, Extent.SizeSquared(), Extent };
}