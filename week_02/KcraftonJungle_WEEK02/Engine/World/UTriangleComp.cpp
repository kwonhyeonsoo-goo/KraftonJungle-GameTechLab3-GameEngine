#include "UTriangleComp.h"
UTriangleComp::UTriangleComp() : UPrimitiveComponent(EPrimitiveShape::Triangle)
{
    BoundsMin = FVector(-0.5f, -0.5f, -0.1f); // 조금 더 두껍게
    BoundsMax = FVector(0.5f, 0.5f, 0.1f);
}

UClass* UTriangleComp::StaticClass()
{
    static UClass ClassInfo("Triangle", UPrimitiveComponent::StaticClass());
    return &ClassInfo;
}

UClass* UTriangleComp::GetClass() const
{
    return UTriangleComp::StaticClass();
}
