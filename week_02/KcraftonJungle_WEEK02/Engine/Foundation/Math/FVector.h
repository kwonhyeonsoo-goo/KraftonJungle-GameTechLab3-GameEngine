#pragma once
#include <cmath>


// Z-up ±âÁØ
// Right   = +X
// Forward = +Y
// Up      = +Z
struct FVector
{
    float x, y, z;

    FVector(float _x = 0.f, float _y = 0.f, float _z = 0.f)
        : x(_x), y(_y), z(_z)
    {
    }

    static float DotProduct(const FVector& lhs, const FVector& rhs)
    {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    static FVector CrossProduct(const FVector& lhs, const FVector& rhs)
    {
        return FVector(
            lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x
        );
    }

    float Dot(const FVector& rhs) const
    {
        return DotProduct(*this, rhs);
    }

    FVector Cross(const FVector& rhs) const
    {
        return CrossProduct(*this, rhs);
    }

    FVector operator+(const FVector& rhs) const
    {
        return FVector(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    FVector& operator+=(const FVector& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    FVector operator-(const FVector& rhs) const
    {
        return FVector(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    FVector operator-() const
    {
        return FVector(-x, -y, -z);
    }

    FVector& operator-=(const FVector& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    FVector operator*(float rhs) const
    {
        return FVector(x * rhs, y * rhs, z * rhs);
    }

    friend FVector operator*(float lhs, const FVector& rhs)
    {
        return rhs * lhs;
    }

    FVector& operator*=(float rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        return *this;
    }

    FVector operator/(float rhs) const
    {
        return FVector(x / rhs, y / rhs, z / rhs);
    }

    FVector& operator/=(float rhs)
    {
        x /= rhs;
        y /= rhs;
        z /= rhs;
        return *this;
    }

    bool operator==(const FVector& rhs) const
    {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

    bool operator!=(const FVector& rhs) const
    {
        return !(*this == rhs);
    }

    float LengthSquare() const
    {
        return x * x + y * y + z * z;
    }

    float LengthSquared() const
    {
        return LengthSquare();
    }

    float Length() const
    {
        return std::sqrt(LengthSquare());
    }

    FVector& Normalize()
    {
        const float len = Length();
        if (len > 0.000001f)
        {
            x /= len;
            y /= len;
            z /= len;
        }
        return *this;
    }

    FVector Normalized() const
    {
        const float len = Length();
        if (len > 0.000001f)
        {
            return FVector(x / len, y / len, z / len);
        }
        return FVector::Zero;
    }

    bool IsNearlyZero(float epsilon = 0.0001f) const
    {
        return std::fabs(x) <= epsilon
            && std::fabs(y) <= epsilon
            && std::fabs(z) <= epsilon;
    }

    static bool NearlyEqual(const FVector& a, const FVector& b, float epsilon = 0.0001f)
    {
        return std::fabs(a.x - b.x) <= epsilon
            && std::fabs(a.y - b.y) <= epsilon
            && std::fabs(a.z - b.z) <= epsilon;
    }

    float GetX() const { return x; }
    float GetY() const { return y; }
    float GetZ() const { return z; }

    static const FVector Zero;
    static const FVector One;

    static const FVector Right;    // +X
    static const FVector Forward;  // +Y
    static const FVector Up;       // +Z
};