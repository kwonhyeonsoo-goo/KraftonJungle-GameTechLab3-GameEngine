#include "UCubeComp.h"
#include "../Mesh/Cube.h"

UCubeComp::UCubeComp() : UPrimitiveComponent(EPrimitiveShape::Cube)
{
    Vertices = cube_vertices;
    NumVertices = sizeof(cube_vertices) / sizeof(FVertexSimple);
}

UClass* UCubeComp::StaticClass()
{
    static UClass ClassInfo("Cube", UPrimitiveComponent::StaticClass());
    return &ClassInfo;
}

UClass* UCubeComp::GetClass() const
{
    return UCubeComp::StaticClass();
}