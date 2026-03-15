#pragma once
#include <cmath>
#include "FMatrix.h"

struct FVector4 {

	float x, y, z, w;

	FVector4()
		: x(0.f), y(0.f), z(0.f), w(0.f) {
	}

	FVector4(float _x = 0.f, float _y = 0.f, float _z = 0.f, float _w = 0.f)
		: x(_x), y(_y), z(_z), w(_w) {
	}

	FVector4(const FVector& _v, float _w = 0.f)
		: x(_v.x), y(_v.y), z(_v.z), w(_w) {
	}

	FVector4 operator-(const FVector4& rhs) const
	{
		return FVector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
	}

	float Dot() const {
		return (x * x + y * y + z * z + w * w);
	}

	float Dot(const FVector4& Other) const {
		return (x * Other.x + y * Other.y + z * Other.z + w * Other.w);
	}

	float Length() const
	{
		return sqrtf(Dot());
	}

	FVector ToVector() const {
		return FVector(x, y, z);
	}

	FVector4& Normalize()
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

	FVector4 operator*(const float& Other) const {
		return FVector4(x * Other, y * Other, z * Other, w * Other);
	}

	FVector4 operator*(const FMatrix& rhs) const {
		return FVector4(
			x * rhs.M[0][0] + y * rhs.M[1][0] + z * rhs.M[2][0] + w * rhs.M[3][0],
			x * rhs.M[0][1] + y * rhs.M[1][1] + z * rhs.M[2][1] + w * rhs.M[3][1],
			x * rhs.M[0][2] + y * rhs.M[1][2] + z * rhs.M[2][2] + w * rhs.M[3][2],
			x * rhs.M[0][3] + y * rhs.M[1][3] + z * rhs.M[2][3] + w * rhs.M[3][3]
		);
	}
};

