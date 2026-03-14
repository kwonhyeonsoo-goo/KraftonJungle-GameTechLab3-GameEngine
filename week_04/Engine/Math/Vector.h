#pragma once

#include "EngineAPI.h"

struct ENGINE_API FVector
{
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;

    FVector() = default;
    FVector(const float InX, const float InY, const float InZ) : X(InX), Y(InY), Z(InZ) {}
	FVector(const DirectX::XMFLOADT3 )

    FVector operator+(const FVector& Other) const;
    FVector operator-(const FVector& Other) const;
    FVector operator*(float Scalar) const;

    float Dot(const FVector& Other) const;
    FVector Cross(const FVector& Other) const;
    float Length() const;
    FVector Normalize() const;
};
