#include "Picker.h"

FRay CPicker::ScreenToRay(int ScreenX, int ScreenY, int ScreenWidth, int ScreenHeight,
                           const FMatrix& ViewMatrix, const FMatrix& ProjMatrix) const
{
    // TODO: Screen → NDC → View → World 역변환
    return {};
}

bool CPicker::RayTriangleIntersect(const FRay& Ray,
                                    const FVector& V0, const FVector& V1, const FVector& V2,
                                    float& OutDistance) const
{
    // TODO: Möller–Trumbore intersection algorithm
    return false;
}
