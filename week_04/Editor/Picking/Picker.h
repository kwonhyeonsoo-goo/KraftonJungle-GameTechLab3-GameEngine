#pragma once

#include "Math/Vector.h"
#include "Math/Matrix.h"

struct FRay
{
    FVector Origin;
    FVector Direction;
};

class CPicker
{
public:
    // 스크린 좌표 → 월드 레이 변환 (Deprojection)
    FRay ScreenToRay(int ScreenX, int ScreenY, int ScreenWidth, int ScreenHeight,
                     const FMatrix& ViewMatrix, const FMatrix& ProjMatrix) const;

    // Möller–Trumbore 알고리즘: 레이-삼각형 교차 검사
    bool RayTriangleIntersect(const FRay& Ray,
                              const FVector& V0, const FVector& V1, const FVector& V2,
                              float& OutDistance) const;
};
