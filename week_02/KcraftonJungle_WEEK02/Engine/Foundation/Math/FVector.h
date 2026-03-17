#pragma once
#include <cmath>
#include <algorithm>

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

    static float ClampFloat(float value, float minValue, float maxValue)
    {
        return (std::max)(minValue, (std::min)(value, maxValue));
    }

    FVector Clamped(float minValue, float maxValue) const
    {
        return FVector(
            ClampFloat(x, minValue, maxValue),
            ClampFloat(y, minValue, maxValue),
            ClampFloat(z, minValue, maxValue)
        );
    }

    FVector& Clamp(float minValue, float maxValue)
    {
        x = ClampFloat(x, minValue, maxValue);
        y = ClampFloat(y, minValue, maxValue);
        z = ClampFloat(z, minValue, maxValue);
        return *this;
    }

    FVector Clamped(const FVector& minValue, const FVector& maxValue) const
    {
        return FVector(
            ClampFloat(x, minValue.x, maxValue.x),
            ClampFloat(y, minValue.y, maxValue.y),
            ClampFloat(z, minValue.z, maxValue.z)
        );
    }

    FVector& Clamp(const FVector& minValue, const FVector& maxValue)
    {
        x = ClampFloat(x, minValue.x, maxValue.x);
        y = ClampFloat(y, minValue.y, maxValue.y);
        z = ClampFloat(z, minValue.z, maxValue.z);
        return *this;
    }

    FVector Clamped(const FVector& minValue, float maxValue) const
    {
        return FVector(
            ClampFloat(x, minValue.x, maxValue),
            ClampFloat(y, minValue.y, maxValue),
            ClampFloat(z, minValue.z, maxValue)
        );
    }

    FVector& Clamp(const FVector& minValue, float maxValue)
    {
        x = ClampFloat(x, minValue.x, maxValue);
        y = ClampFloat(y, minValue.y, maxValue);
        z = ClampFloat(z, minValue.z, maxValue);
        return *this;
    }

    FVector Clamped(float minValue, const FVector& maxValue) const
    {
        return FVector(
            ClampFloat(x, minValue, maxValue.x),
            ClampFloat(y, minValue, maxValue.y),
            ClampFloat(z, minValue, maxValue.z)
        );
    }

    FVector& Clamp(float minValue, const FVector& maxValue)
    {
        x = ClampFloat(x, minValue, maxValue.x);
        y = ClampFloat(y, minValue, maxValue.y);
        z = ClampFloat(z, minValue, maxValue.z);
        return *this;
    }

    FVector ClampedMin(float minValue) const
    {
        return FVector(
            (std::max)(x, minValue),
            (std::max)(y, minValue),
            (std::max)(z, minValue)
        );
    }

    FVector& ClampMin(float minValue)
    {
        x = (std::max)(x, minValue);
        y = (std::max)(y, minValue);
        z = (std::max)(z, minValue);
        return *this;
    }

    FVector ClampedMin(const FVector& minValue) const
    {
        return FVector(
            (std::max)(x, minValue.x),
            (std::max)(y, minValue.y),
            (std::max)(z, minValue.z)
        );
    }

    FVector& ClampMin(const FVector& minValue)
    {
        x = (std::max)(x, minValue.x);
        y = (std::max)(y, minValue.y);
        z = (std::max)(z, minValue.z);
        return *this;
    }

    FVector ClampedMax(float maxValue) const
    {
        return FVector(
            (std::min)(x, maxValue),
            (std::min)(y, maxValue),
            (std::min)(z, maxValue)
        );
    }

    FVector& ClampMax(float maxValue)
    {
        x = (std::min)(x, maxValue);
        y = (std::min)(y, maxValue);
        z = (std::min)(z, maxValue);
        return *this;
    }

    FVector ClampedMax(const FVector& maxValue) const
    {
        return FVector(
            (std::min)(x, maxValue.x),
            (std::min)(y, maxValue.y),
            (std::min)(z, maxValue.z)
        );
    }

    FVector& ClampMax(const FVector& maxValue)
    {
        x = (std::min)(x, maxValue.x);
        y = (std::min)(y, maxValue.y);
        z = (std::min)(z, maxValue.z);
        return *this;
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