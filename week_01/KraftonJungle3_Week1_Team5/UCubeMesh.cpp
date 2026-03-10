#include "UCubeMesh.h"
#include "Cube.h"

bool UCubeMesh::CreateCube(ID3D11Device* device)
{
    if (device == nullptr)
    {
        return false;
    }

    UINT numVerticesCube = sizeof(cube_vertices) / sizeof(FVertexColor);

    SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    return CreateVertexBuffer(device, cube_vertices, sizeof(FVertexColor), numVerticesCube);
}