#include "Math/Vector.h"

const FVector FVector::ZeroVector(0.f, 0.f, 0.f);
const FVector FVector::OneVector(1.0f, 1.0f, 1.0f);
const FVector FVector::UpVector(0.0f, 0.0f, 1.0f);
const FVector FVector::DownVector(0.0f, 0.0f, -1.0f);
const FVector FVector::ForwardVector(1.0f, 0.0f, 0.0f);
const FVector FVector::BackwardVector(-1.0f, 0.0f, 0.0f);
const FVector FVector::RightVector(0.0f, 1.0f, 0.0f);
const FVector FVector::LeftVector(0.0f, -1.0f, 0.0f);
const FVector FVector::XAxisVector(1.0f, 0.0f, 0.0f);
const FVector FVector::YAxisVector(0.0f, 1.0f, 0.0f);
const FVector FVector::ZAxisVector(0.0f, 0.0f, 1.0f);

//FVector FVector::operator+(const FVector& Other) const
//{
//	return { X + Other.X, Y + Other.Y, Z + Other.Z };
//}
//
//FVector FVector::operator-(const FVector& Other) const
//{
//	return { X - Other.X, Y - Other.Y, Z - Other.Z };
//}
//
//FVector FVector::operator*(float Scalar) const
//{
//	return { X * Scalar, Y * Scalar, Z * Scalar };
//}
//
//float FVector::Dot(const FVector& Other) const
//{
//	return X * Other.X + Y * Other.Y + Z * Other.Z;
//}
//
//FVector FVector::Cross(const FVector& Other) const
//{
//	return {
//		Y * Other.Z - Z * Other.Y,
//		Z * Other.X - X * Other.Z,
//		X * Other.Y - Y * Other.X
//	};
//}
//
//float FVector::Length() const
//{
//	return sqrtf(X * X + Y * Y + Z * Z);
//}
//
//FVector FVector::Normalize() const
//{
//	float Len = Length();
//	if (Len < 1e-6f) return { 0.0f, 0.0f, 0.0f };
//	return { X / Len, Y / Len, Z / Len };
//}
