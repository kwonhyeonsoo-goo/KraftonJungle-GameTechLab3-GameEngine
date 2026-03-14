#include "UPlaneComp.h"

UPlaneComp::UPlaneComp() : UPrimitiveComponent(EPrimitiveShape::Plane)
{
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