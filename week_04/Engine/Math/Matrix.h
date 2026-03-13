#pragma once

#include "EngineAPI.h"
#include "Math/Vector.h"
#include "Math/Vector4.h"

struct ENGINE_API FMatrix
{
    float M[4][4] = {};

    FMatrix() = default;

    static FMatrix Identity();

    FMatrix operator*(const FMatrix& Other) const;
    FVector4 TransformVector4(const FVector4& V) const;

    FMatrix Transpose() const;
    FMatrix Inverse() const;

    // MVP 변환용 팩토리 메서드
    static FMatrix Translation(float X, float Y, float Z);
    static FMatrix RotationX(float AngleRad);
    static FMatrix RotationY(float AngleRad);
    static FMatrix RotationZ(float AngleRad);
    static FMatrix Scale(float X, float Y, float Z);

    static FMatrix LookAt(const FVector& Eye, const FVector& Target, const FVector& Up);
    static FMatrix Perspective(float FOV, float AspectRatio, float Near, float Far);
};
