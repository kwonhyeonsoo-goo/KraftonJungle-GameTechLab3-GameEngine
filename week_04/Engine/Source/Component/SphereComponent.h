#pragma once
#include "PrimitiveComponent.h"

class ENGINE_API USphereComponent : public UPrimitiveComponent
{
public:
	static UClass* StaticClass();

	USphereComponent();
	USphereComponent(UClass* InClass, const FString& InName, UObject* InOuter = nullptr);

	FBoxSphereBounds GetWorldBoundsForAABB() const override
	{
		FVector Center = GetWorldLocation();
		FTransform T = GetRelativeTransform();
		FVector S = T.GetScale3D();
		FVector WorldBoxExtent = FVector::Multiply(LocalBoxExtent, S);

		return { Center, WorldBoxExtent.SizeSquared(), WorldBoxExtent };
	}
};
