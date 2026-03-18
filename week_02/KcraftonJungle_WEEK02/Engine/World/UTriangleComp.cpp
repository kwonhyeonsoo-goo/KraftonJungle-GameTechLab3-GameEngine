#include "UTriangleComp.h"
#include "../Mesh/Triangle.h"

UTriangleComp::UTriangleComp() : UPrimitiveComponent(EPrimitiveShape::Triangle)
{
    BoundsMin = FVector(-0.5f, -0.5f, -0.1f); // 조금 더 두껍게
    BoundsMax = FVector(0.5f, 0.5f, 0.1f);
    Vertices = triangle_vertices;
    NumVertices = sizeof(triangle_vertices) / sizeof(FVertexSimple);
}

UClass* UTriangleComp::StaticClass()
{
    static UClass ClassInfo("Triangle", UPrimitiveComponent::StaticClass(), []() -> std::unique_ptr<UObject> { return std::make_unique<UTriangleComp>(); });
    return &ClassInfo;
}

UClass* UTriangleComp::GetClass() const
{
    return UTriangleComp::StaticClass();
}
