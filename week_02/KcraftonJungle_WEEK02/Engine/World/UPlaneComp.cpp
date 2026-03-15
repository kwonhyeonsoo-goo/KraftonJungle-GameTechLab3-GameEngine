#include "UPlaneComp.h"

UPlaneComp::UPlaneComp() : UPrimitiveComponent(EPrimitiveShape::Plane)
{
    BoundsMin = FVector(-0.5f, -0.5f, -0.1f); // 조금 더 두껍게
    BoundsMax = FVector(0.5f, 0.5f, 0.1f);
}

UClass* UPlaneComp::StaticClass()
{
    static UClass ClassInfo("Plane", UPrimitiveComponent::StaticClass());
    return &ClassInfo;
}

UClass* UPlaneComp::GetClass() const
{
    return UPlaneComp::StaticClass();
}