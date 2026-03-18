#include "USphereComp.h"

USphereComp::USphereComp() : UPrimitiveComponent(EPrimitiveShape::Sphere)
{
    bVisible = true;
    BoundsMin = FVector(-1.0f, -1.0f, -1.0f);
    BoundsMax = FVector(1.0f, 1.0f, 1.0f);
}

UClass* USphereComp::StaticClass()
{
    static UClass ClassInfo("Sphere", UPrimitiveComponent::StaticClass(), []() -> std::unique_ptr<UObject> { return std::make_unique<USphereComp>(); });
    return &ClassInfo;
}

UClass* USphereComp::GetClass() const
{
    return USphereComp::StaticClass();
}