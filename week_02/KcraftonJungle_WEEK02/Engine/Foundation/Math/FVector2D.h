#pragma once
#include <cmath>

struct FVector2D
{
    float x;
    float y;

    static const FVector2D Zero;
    static const FVector2D One;

    FVector2D()
        : x(0.0f), y(0.0f)
    {
    }

    FVector2D(float inX, float inY)
        : x(inX), y(inY)
    {
    }

    FVector2D operator+(const FVector2D& rhs) const
    {
        return FVector2D(x + rhs.x, y + rhs.y);
    }

    FVector2D operator-(const FVector2D& rhs) const
    {
        return FVector2D(x - rhs.x, y - rhs.y);
    }

    FVector2D operator*(float scalar) const
    {
        return FVector2D(x * scalar, y * scalar);
    }

    FVector2D operator/(float scalar) const
    {
        return FVector2D(x / scalar, y / scalar);
    }

    FVector2D& operator+=(const FVector2D& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    FVector2D& operator-=(const FVector2D& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    FVector2D& operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    FVector2D& operator/=(float scalar)
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    bool operator==(const FVector2D& rhs) const
    {
        return x == rhs.x && y == rhs.y;
    }

    bool operator!=(const FVector2D& rhs) const
    {
        return !(*this == rhs);
    }

    float Dot(const FVector2D& rhs) const
    {
        return x * rhs.x + y * rhs.y;
    }

    float LengthSquared() const
    {
        return x * x + y * y;
    }

    float Length() const
    {
        return std::sqrt(LengthSquared());
    }

    FVector2D Normalized() const
    {
        const float len = Length();
        if (len <= 0.000001f)
        {
            return FVector2D::Zero;
        }

        return FVector2D(x / len, y / len);
    }

    void Normalize()
    {
        const float len = Length();
        if (len <= 0.000001f)
        {
            x = 0.0f;
            y = 0.0f;
            return;
        }

        x /= len;
        y /= len;
    }
};