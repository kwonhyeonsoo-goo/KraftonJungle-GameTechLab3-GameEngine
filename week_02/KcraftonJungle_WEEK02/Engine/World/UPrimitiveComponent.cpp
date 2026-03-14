#include "UPrimitiveComponent.h"

UPrimitiveComponent::UPrimitiveComponent() : Shape(EPrimitiveShape::Cube)
{
    bVisible = true;
    BoundsMin = FVector(-0.5f, -0.5f, -0.5f);
    BoundsMax = FVector(0.5f, 0.5f, 0.5f);
}

UPrimitiveComponent::UPrimitiveComponent(EPrimitiveShape shape) : Shape(shape)
{
    bVisible = true;
    BoundsMin = FVector(-0.5f, -0.5f, -0.5f);
    BoundsMax = FVector(0.5f, 0.5f, 0.5f);
}

UClass* UPrimitiveComponent::StaticClass()
{
	static UClass ClassInfo("PrimitiveComponent", USceneComponent::StaticClass());
	return &ClassInfo;
}

UClass* UPrimitiveComponent::GetClass() const
{
	return UPrimitiveComponent::StaticClass();
}

FVector UPrimitiveComponent::GetBoundMin() const
{
    return BoundsMin;
}

void UPrimitiveComponent::SetBoundMin(FVector min)
{
    BoundsMin = min;
}

FVector UPrimitiveComponent::GetBoundMax() const
{
    return BoundsMax;
}

void UPrimitiveComponent::SetBoundMax(FVector max)
{
    BoundsMax = max;
}