#include "UTriangleComp.h"
UTriangleComp::UTriangleComp() : UPrimitiveComponent(EPrimitiveShape::Triangle)
{
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
