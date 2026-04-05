#include "Frustum.h"

void FFrustum::Update(const FMatrix& ViewProjection)
{
    const FMatrix& M = ViewProjection;

    // 6개 평면 업데이트 (전통적인 방식 유지)
    Planes[0].Normal.X = M.M[0][3] + M.M[0][0];
    Planes[0].Normal.Y = M.M[1][3] + M.M[1][0];
    Planes[0].Normal.Z = M.M[2][3] + M.M[2][0];
    Planes[0].Distance = M.M[3][3] + M.M[3][0];

    Planes[1].Normal.X = M.M[0][3] - M.M[0][0];
    Planes[1].Normal.Y = M.M[1][3] - M.M[1][0];
    Planes[1].Normal.Z = M.M[2][3] - M.M[2][0];
    Planes[1].Distance = M.M[3][3] - M.M[3][0];

    Planes[2].Normal.X = M.M[0][3] + M.M[0][1];
    Planes[2].Normal.Y = M.M[1][3] + M.M[1][1];
    Planes[2].Normal.Z = M.M[2][3] + M.M[2][1];
    Planes[2].Distance = M.M[3][3] + M.M[3][1];

    Planes[3].Normal.X = M.M[0][3] - M.M[0][1];
    Planes[3].Normal.Y = M.M[1][3] - M.M[1][1];
    Planes[3].Normal.Z = M.M[2][3] - M.M[2][1];
    Planes[3].Distance = M.M[3][3] - M.M[3][1];

    Planes[4].Normal.X = M.M[0][2];
    Planes[4].Normal.Y = M.M[1][2];
    Planes[4].Normal.Z = M.M[2][2];
    Planes[4].Distance = M.M[3][2];

    Planes[5].Normal.X = M.M[0][3] - M.M[0][2];
    Planes[5].Normal.Y = M.M[1][3] - M.M[1][2];
    Planes[5].Normal.Z = M.M[2][3] - M.M[2][2];
    Planes[5].Distance = M.M[3][3] - M.M[3][2];

    for (int i = 0; i < 6; ++i)
    {
        float InvLen = 1.0f / Planes[i].Normal.Size();
        Planes[i].Normal *= InvLen;
        Planes[i].Distance *= InvLen;
    }

    // SIMD 데이터를 위한 재배치
    using namespace DirectX;
    float PX[8], PY[8], PZ[8], PD[8];

    for (int i = 0; i < 6; ++i)
    {
        PX[i] = Planes[i].Normal.X;
        PY[i] = Planes[i].Normal.Y;
        PZ[i] = Planes[i].Normal.Z;
        PD[i] = Planes[i].Distance;
    }

    // 나머지 슬롯 (6, 7)을 "항상 통과"하도록 큰 값으로 채움
    for (int i = 6; i < 8; ++i)
    {
        PX[i] = 0.0f; PY[i] = 0.0f; PZ[i] = 0.0f;
        PD[i] = 1000000.0f; // 매우 큰 양수 값
    }

    PlanesX[0] = XMLoadFloat4((const XMFLOAT4*)&PX[0]);
    PlanesY[0] = XMLoadFloat4((const XMFLOAT4*)&PY[0]);
    PlanesZ[0] = XMLoadFloat4((const XMFLOAT4*)&PZ[0]);
    PlanesD[0] = XMLoadFloat4((const XMFLOAT4*)&PD[0]);

    PlanesX[1] = XMLoadFloat4((const XMFLOAT4*)&PX[4]);
    PlanesY[1] = XMLoadFloat4((const XMFLOAT4*)&PY[4]);
    PlanesZ[1] = XMLoadFloat4((const XMFLOAT4*)&PZ[4]);
    PlanesD[1] = XMLoadFloat4((const XMFLOAT4*)&PD[4]);
}
