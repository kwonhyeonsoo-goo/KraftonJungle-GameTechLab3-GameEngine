#include "Frustum.h"

void FFrustum::Update(const FMatrix& ViewProjection)
{
    const FMatrix& M = ViewProjection;

    // Left Plane
    Planes[0].Normal.X = M.M[0][3] + M.M[0][0];
    Planes[0].Normal.Y = M.M[1][3] + M.M[1][0];
    Planes[0].Normal.Z = M.M[2][3] + M.M[2][0];
    Planes[0].Distance = M.M[3][3] + M.M[3][0];

    // Right Plane
    Planes[1].Normal.X = M.M[0][3] - M.M[0][0];
    Planes[1].Normal.Y = M.M[1][3] - M.M[1][0];
    Planes[1].Normal.Z = M.M[2][3] - M.M[2][0];
    Planes[1].Distance = M.M[3][3] - M.M[3][0];

    // Bottom Plane
    Planes[2].Normal.X = M.M[0][3] + M.M[0][1];
    Planes[2].Normal.Y = M.M[1][3] + M.M[1][1];
    Planes[2].Normal.Z = M.M[2][3] + M.M[2][1];
    Planes[2].Distance = M.M[3][3] + M.M[3][1];

    // Top Plane
    Planes[3].Normal.X = M.M[0][3] - M.M[0][1];
    Planes[3].Normal.Y = M.M[1][3] - M.M[1][1];
    Planes[3].Normal.Z = M.M[2][3] - M.M[2][1];
    Planes[3].Distance = M.M[3][3] - M.M[3][1];

    // Near Plane (D3D 기준: 0 to w)
    Planes[4].Normal.X = M.M[0][2];
    Planes[4].Normal.Y = M.M[1][2];
    Planes[4].Normal.Z = M.M[2][2];
    Planes[4].Distance = M.M[3][2];

    // Far Plane (D3D 기준: w - z)
    Planes[5].Normal.X = M.M[0][3] - M.M[0][2];
    Planes[5].Normal.Y = M.M[1][3] - M.M[1][2];
    Planes[5].Normal.Z = M.M[2][3] - M.M[2][2];
    Planes[5].Distance = M.M[3][3] - M.M[3][2];

    // 중요: 모든 평면의 노멀을 정규화(Normalize)해야 GetDistance가 정확한 거리를 반환합니다.
    for (int i = 0; i < 6; ++i)
    {
        float InvLen = 1.0f / Planes[i].Normal.Size();
        Planes[i].Normal *= InvLen;
        Planes[i].Distance *= InvLen;
    }
}
