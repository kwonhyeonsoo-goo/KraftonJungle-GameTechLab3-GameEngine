#include "UCubeComp.h"
#include "../Mesh/Cube.h"

UCubeComp::UCubeComp() : UPrimitiveComponent(EPrimitiveShape::Cube)
{
    Vertices = cube_vertices;
    NumVertices = sizeof(cube_vertices) / sizeof(FVertexSimple);
}

UClass* UCubeComp::StaticClass()
{
    static UClass ClassInfo("Cube", UPrimitiveComponent::StaticClass(), []() -> std::unique_ptr<UObject> { return std::make_unique<UCubeComp>(); });
    return &ClassInfo;
}

UClass* UCubeComp::GetClass() const
{
    return UCubeComp::StaticClass();
}