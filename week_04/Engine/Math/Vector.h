#pragma once

#include "EngineAPI.h"
#include <cmath>

template <typename T>
struct TVector
{
	T X = static_cast<T>(0);
	T Y = static_cast<T>(0);
	T Z = static_cast<T>(0);

	TVector() = default;
	TVector(T InX, T InY, T InZ) : X(InX), Y(InY), Z(InZ) {}

	TVector operator+(const TVector& Other) const
	{
		return { X + Other.X, Y + Other.Y, Z + Other.Z };
	}

	TVector operator-(const TVector& Other) const
	{
		return { X - Other.X, Y - Other.Y, Z - Other.Z };
	}

	TVector operator*(T Scalar) const
	{
		return { X * Scalar, Y * Scalar, Z * Scalar };
	}

	T Dot(const TVector& Other) const
	{
		return X * Other.X + Y * Other.Y + Z * Other.Z;
	}

	TVector Cross(const TVector& Other) const
	{
		return {
			Y * Other.Z - Z * Other.Y,
			Z * Other.X - X * Other.Z,
			X * Other.Y - Y * Other.X
		};
	}

	T Length() const
	{
		return static_cast<T>(std::sqrt(X * X + Y * Y + Z * Z));
	}

	TVector Normalize() const
	{
		T Len = Length();
		if (Len < static_cast<T>(1e-6)) return {};
		return { X / Len, Y / Len, Z / Len };
	}

	static const TVector ZeroVector;
	static const TVector OneVector;
	static const TVector ForwardVector;
	static const TVector RightVector;
	static const TVector UpVector;
};

template <typename T>
inline const TVector<T> TVector<T>::ZeroVector = { static_cast<T>(0), static_cast<T>(0), static_cast<T>(0) };

template <typename T>
inline const TVector<T> TVector<T>::OneVector = { static_cast<T>(1), static_cast<T>(1), static_cast<T>(1) };

template <typename T>
inline const TVector<T> TVector<T>::ForwardVector = { static_cast<T>(1), static_cast<T>(0), static_cast<T>(0) };

template <typename T>
inline const TVector<T> TVector<T>::RightVector = { static_cast<T>(0), static_cast<T>(1), static_cast<T>(0) };

template <typename T>
inline const TVector<T> TVector<T>::UpVector = { static_cast<T>(0), static_cast<T>(0), static_cast<T>(1) };

using FVector = TVector<float>;
