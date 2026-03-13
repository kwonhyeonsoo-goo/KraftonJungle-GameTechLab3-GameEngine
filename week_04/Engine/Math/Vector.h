#pragma once

struct FVector
{
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;

    FVector() = default;
    FVector(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}

    FVector operator+(const FVector& Other) const;
    FVector operator-(const FVector& Other) const;
    FVector operator*(float Scalar) const;

    float Dot(const FVector& Other) const;
    FVector Cross(const FVector& Other) const;
    float Length() const;
    FVector Normalize() const;
};
