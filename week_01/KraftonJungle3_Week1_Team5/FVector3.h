#pragma once
#include <cmath>

struct FVector3
{
	float x, y, z;
	FVector3(float _x = 0.f, float _y = 0.f, float _z = 0.f)
		: x(_x), y(_y), z(_z) {
	}

	static float DotProduct(const FVector3& lhs, const FVector3& rhs)
	{
		return (lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z);
	}

	static FVector3 CrossProduct(const FVector3& lhs, const FVector3& rhs)
	{
		return {
			lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.z * rhs.x - lhs.x * rhs.z,
			lhs.x * rhs.y - lhs.y * rhs.x
		};
	}

	float Dot(const FVector3& rhs)
	{
		return DotProduct(*this, rhs);
	}

	FVector3 Cross(const FVector3& rhs)
	{
		return CrossProduct(*this, rhs);
	}

	FVector3 operator+(const FVector3& rhs) const
	{
		return { x + rhs.x, y + rhs.y, z + rhs.z };
	}
	FVector3& operator+=(const FVector3& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}
	FVector3 operator-(const FVector3& rhs) const
	{
		return { x - rhs.x, y - rhs.y, z - rhs.z };
	}

	FVector3 operator-() const
	{
		return { -x, -y, -z };
	}

	FVector3& operator-=(const FVector3& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}

	FVector3 operator*(const float rhs) const
	{
		return { x * rhs, y * rhs, z * rhs };
	}

	friend FVector3 operator*(const float lhs, const FVector3& rhs)
	{
		return rhs * lhs;
	}

	FVector3& operator*=(const float rhs)
	{
		x *= rhs;
		y *= rhs;
		z *= rhs;
		return *this;
	}

	FVector3 operator/(const float rhs) const
	{
		return { x / rhs, y / rhs, z / rhs };
	}

	FVector3& operator/=(const float rhs)
	{
		x /= rhs;
		y /= rhs;
		z /= rhs;
		return *this;
	}

	float LengthSquare() const
	{
		return (x * x + y * y + z * z);
	}

	float Length() const
	{
		return sqrtf(LengthSquare());
	}

	FVector3& Normalize()
	{
		float Len = Length();
		if (Len > 0.0f)
		{
			x /= Len;
			y /= Len;
			z /= Len;
		}
		return *this;
	}

	float GetX() { return x; }
	float GetY() { return y; }
	float GetZ() { return z; }
};
