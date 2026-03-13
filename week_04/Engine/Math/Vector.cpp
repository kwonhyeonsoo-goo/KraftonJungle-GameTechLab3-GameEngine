#include "Math/Vector.h"
#include <cmath>

FVector FVector::operator+(const FVector& Other) const
{
	return { X + Other.X, Y + Other.Y, Z + Other.Z };
}

FVector FVector::operator-(const FVector& Other) const
{
	return { X - Other.X, Y - Other.Y, Z - Other.Z };
}

FVector FVector::operator*(float Scalar) const
{
	return { X * Scalar, Y * Scalar, Z * Scalar };
}

float FVector::Dot(const FVector& Other) const
{
	return X * Other.X + Y * Other.Y + Z * Other.Z;
}

FVector FVector::Cross(const FVector& Other) const
{
	return {
		Y * Other.Z - Z * Other.Y,
		Z * Other.X - X * Other.Z,
		X * Other.Y - Y * Other.X
	};
}

float FVector::Length() const
{
	return sqrtf(X * X + Y * Y + Z * Z);
}

FVector FVector::Normalize() const
{
	float Len = Length();
	if (Len < 1e-6f) return { 0.0f, 0.0f, 0.0f };
	return { X / Len, Y / Len, Z / Len };
}
