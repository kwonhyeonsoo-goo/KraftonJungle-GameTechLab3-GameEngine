#include "USphereMesh.h"
#include "Sphere.h"


bool USphereMesh::CreateSphere(ID3D11Device* device)
{
    if (device == nullptr)
    {
        return false;
    }

    UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexColor);

    SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    return CreateVertexBuffer(device, sphere_vertices, sizeof(FVertexColor), numVerticesSphere);
}
