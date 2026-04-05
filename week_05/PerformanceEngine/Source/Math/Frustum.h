#pragma once

#include "Math/BoundingBox.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"
#include <DirectXMath.h>

struct FPlane
{
	FVector Normal = FVector::ForwardVector;
	float Distance = 0.0f;

	float GetDistance(const FVector& Point) const
	{
		return FVector::DotProduct(Normal, Point) + Distance;
	}
};

struct FFrustum
{
	FPlane Planes[6];

	// SIMD 최적화용 데이터 (평면 6개를 4개/2개로 나누어 저장)
	DirectX::XMVECTOR PlanesX[2]; // [P0-P3.X], [P4-P5.X]
	DirectX::XMVECTOR PlanesY[2];
	DirectX::XMVECTOR PlanesZ[2];
	DirectX::XMVECTOR PlanesD[2];

	void Update(const FMatrix& ViewProjection);

	inline bool IsOutSide(const FBoundingBox& Box) const
	{
		using namespace DirectX;
		
		XMVECTOR BMin = XMLoadFloat3((const XMFLOAT3*)&Box.Min);
		XMVECTOR BMax = XMLoadFloat3((const XMFLOAT3*)&Box.Max);

		XMVECTOR MinX = XMVectorSplatX(BMin);
		XMVECTOR MinY = XMVectorSplatY(BMin);
		XMVECTOR MinZ = XMVectorSplatZ(BMin);
		XMVECTOR MaxX = XMVectorSplatX(BMax);
		XMVECTOR MaxY = XMVectorSplatY(BMax);
		XMVECTOR MaxZ = XMVectorSplatZ(BMax);

		for (int i = 0; i < 2; ++i)
		{
			XMVECTOR PX = XMVectorSelect(MinX, MaxX, XMVectorGreaterOrEqual(PlanesX[i], g_XMZero));
			XMVECTOR PY = XMVectorSelect(MinY, MaxY, XMVectorGreaterOrEqual(PlanesY[i], g_XMZero));
			XMVECTOR PZ = XMVectorSelect(MinZ, MaxZ, XMVectorGreaterOrEqual(PlanesZ[i], g_XMZero));

			XMVECTOR Dist = XMVectorMultiply(PlanesX[i], PX);
			Dist = XMVectorMultiplyAdd(PlanesY[i], PY, Dist);
			Dist = XMVectorMultiplyAdd(PlanesZ[i], PZ, Dist);
			Dist = XMVectorAdd(Dist, PlanesD[i]);

			// DirectXMath의 표준 비교 제어 함수 사용
			uint32_t CR;
			XMVectorGreaterR(&CR, g_XMZero, Dist);
			if (XMComparisonAnyTrue(CR)) return true;
		}

		return false;
	}
};
