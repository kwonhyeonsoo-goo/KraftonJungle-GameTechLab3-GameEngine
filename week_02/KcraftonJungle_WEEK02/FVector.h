#pragma once
#include <cmath>


// Structure for a 3D vector
struct FVector
{
	float x, y, z;
	FVector(float _x = 0.f, float _y = 0.f, float _z = 0.f)
		: x(_x), y(_y), z(_z) {
	}

	static float DotProduct(const FVector& lhs, const FVector& rhs)
	{
		return (lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z);
	}

	static FVector CrossProduct(const FVector& lhs, const FVector& rhs)
	{
		return {
			lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.z * rhs.x - lhs.x * rhs.z,
			lhs.x * rhs.y - lhs.y * rhs.x
		};
	}

	float Dot(const FVector& rhs)
	{
		return DotProduct(*this, rhs);
	}

	FVector Cross(const FVector& rhs)
	{
		return CrossProduct(*this, rhs);
	}

	FVector operator+(const FVector& rhs) const
	{
		return { x + rhs.x, y + rhs.y, z + rhs.z };
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
		return { x - rhs.x, y - rhs.y, z - rhs.z };
	}

	FVector operator-() const
	{
		return { -x, -y, -z };
	}

	FVector& operator-=(const FVector& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}

	FVector operator*(const float rhs) const
	{
		return { x * rhs, y * rhs, z * rhs };
	}

	friend FVector operator*(const float lhs, const FVector& rhs)
	{
		return rhs * lhs;
	}

	FVector& operator*=(const float rhs)
	{
		x *= rhs;
		y *= rhs;
		z *= rhs;
		return *this;
	}

	FVector operator/(const float rhs) const
	{
		return { x / rhs, y / rhs, z / rhs };
	}

	FVector& operator/=(const float rhs)
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

	void Normalize()
	{
		float Len = Length();
		if (Len > 0.0f)
		{
			x /= Len;
			y /= Len;
			z /= Len;
		}
	}

	float GetX() { return x; }
	float GetY() { return y; }
	float GetZ() { return z; }
};

