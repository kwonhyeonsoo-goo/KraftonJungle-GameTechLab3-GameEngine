#include "UTextureMesh.h"

struct FVertexTexture
{
    float x, y, z;
    float u, v;
};

bool UTextureMesh::CreateRect(ID3D11Device* device)
{
    return CreateRect(device, 1.0f, 1.0f);
}

bool UTextureMesh::CreateRect(ID3D11Device* device, float halfWidthNdc, float halfHeightNdc)
{
    if (device == nullptr)
    {
        return false;
    }

    FVertexTexture rectVertices[] =
    {
        { -halfWidthNdc, -halfHeightNdc, 0.0f, 0.0f, 1.0f },
        { -halfWidthNdc,  halfHeightNdc, 0.0f, 0.0f, 0.0f },
        {  halfWidthNdc,  halfHeightNdc, 0.0f, 1.0f, 0.0f },
        { -halfWidthNdc, -halfHeightNdc, 0.0f, 0.0f, 1.0f },
        {  halfWidthNdc,  halfHeightNdc, 0.0f, 1.0f, 0.0f },
        {  halfWidthNdc, -halfHeightNdc, 0.0f, 1.0f, 1.0f }
    };

    UINT numVerticesRect = sizeof(rectVertices) / sizeof(FVertexTexture);

    SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    return CreateVertexBuffer(device, rectVertices, sizeof(FVertexTexture), numVerticesRect);
}
