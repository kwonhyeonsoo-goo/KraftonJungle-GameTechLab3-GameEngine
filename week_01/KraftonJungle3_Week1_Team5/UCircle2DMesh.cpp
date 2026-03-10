#include "UCircle2DMesh.h"

#include <cmath>
#include <numbers>
#include <vector>

#include "Circle.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

UCircle2DMesh::UCircle2DMesh() : Radius(0.f), SegmentCount(0)
{
}

bool UCircle2DMesh::CreateCircle(ID3D11Device* device, float radius, UINT segmentCount, DirectX::XMFLOAT4 color)
{
    if (device == nullptr)
    {
        return false;
    }

    UINT numVerticesCircle = sizeof(circle_vertices) / sizeof(FVertexColor);

    SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    return CreateVertexBuffer(device, circle_vertices, sizeof(FVertexColor), numVerticesCircle);
    //if (radius <= 0.f)
    //{
    //    return false;
    //}

    //if (segmentCount < 3)
    //{
    //    return false;
    //}

    //Radius = radius;
    //SegmentCount = segmentCount;

    //std::vector<FVertexColor> vertices;
    //std::vector<UINT> indices;

    //// 중심 정점 1개 + 둘레 정점 (segmentCount + 1)개
    //vertices.reserve(segmentCount + 2);

    //// 중심 정점
    //vertices.push_back({
    //    0.f, 0.f, 0.f,
    //    color.x, color.y, color.z, color.w
    //    });

    //// 둘레 정점
    //for (UINT i = 0; i <= segmentCount; ++i)
    //{
    //    const float t = static_cast<float>(i) / static_cast<float>(segmentCount);
    //    const float angle = t * 2.0f * std::numbers::pi_v<float>;

    //    const float x = std::cos(angle) * radius;
    //    const float y = std::sin(angle) * radius;

    //    vertices.push_back({
    //        x, y, 0.f,
    //        color.x, color.y, color.z, color.w
    //        });
    //}

    //// 삼각형 인덱스
    //// center = 0
    //// ring   = 1 ~ segmentCount+1
    //indices.reserve(segmentCount * 3);

    //for (UINT i = 1; i <= segmentCount; ++i)
    //{
    //    indices.push_back(0);
    //    indices.push_back(i);
    //    indices.push_back(i + 1);
    //}

    //SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //return Create(
    //    device,
    //    vertices.data(),
    //    sizeof(FVertexColor),
    //    static_cast<UINT>(vertices.size()),
    //    indices.data(),
    //    static_cast<UINT>(indices.size())
    //);
}

