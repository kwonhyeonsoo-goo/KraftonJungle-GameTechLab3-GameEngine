#pragma once
#include <cmath>

struct FVector4 {

	float x, y, z, w;


	FVector4(float _x = 0.f, float _y = 0.f, float _z = 0.f, float _w = 0.f)
		: x(_x), y(_y), z(_z), w(_w) {
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


};

