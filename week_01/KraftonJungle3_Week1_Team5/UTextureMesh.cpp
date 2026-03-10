#include "UTextureMesh.h"

struct FVertexTexture
{
    float x, y, z;
    float u, v;
};

FVertexTexture rect_vertices[] =
{
    // triangle 1
    { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f },
    { -1.0f,  1.0f, 0.0f, 0.0f, 0.0f },
    {  1.0f,  1.0f, 0.0f, 1.0f, 0.0f },

    // triangle 2
    { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f },
    {  1.0f,  1.0f, 0.0f, 1.0f, 0.0f },
    {  1.0f, -1.0f, 0.0f, 1.0f, 1.0f }
};

bool UTextureMesh::CreateRect(ID3D11Device* device)
{
    if (device == nullptr)
    {
        return false;
    }

    UINT numVerticesRect = sizeof(rect_vertices) / sizeof(FVertexTexture);

    SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    return CreateVertexBuffer(device, rect_vertices, sizeof(FVertexTexture), numVerticesRect);
}
