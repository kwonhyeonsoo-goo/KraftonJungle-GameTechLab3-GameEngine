#include "UPlaneComp.h"
#include "../Mesh/Rect.h"

UPlaneComp::UPlaneComp() : UPrimitiveComponent(EPrimitiveShape::Plane)
{
    BoundsMin = FVector(-0.5f, -0.5f, -0.1f); // 조금 더 두껍게
    BoundsMax = FVector(0.5f, 0.5f, 0.1f);
    Vertices = rect_vertices;
    NumVertices = sizeof(rect_vertices) / sizeof(FVertexSimple);
}

UClass* UPlaneComp::StaticClass()
{
    static UClass ClassInfo("Plane", UPrimitiveComponent::StaticClass(), []() -> std::unique_ptr<UObject> { return std::make_unique<UPlaneComp>(); });
    return &ClassInfo;
}

UClass* UPlaneComp::GetClass() const
{
    return UPlaneComp::StaticClass();
}