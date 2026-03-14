#include "USphereComp.h"

USphereComp::USphereComp() : UPrimitiveComponent(EPrimitiveShape::Sphere)
{
}

UClass* USphereComp::StaticClass()
{
    static UClass ClassInfo("Sphere", UPrimitiveComponent::StaticClass());
    return &ClassInfo;
}

UClass* USphereComp::GetClass() const
{
    return USphereComp::StaticClass();
}