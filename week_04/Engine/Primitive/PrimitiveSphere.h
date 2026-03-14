#pragma once

#include "Primitive/PrimitiveBase.h"

class ENGINE_API CPrimitiveSphere : public CPrimitiveBase
{
public:
	static const FString Key;
	static const FString FilePath;

	CPrimitiveSphere(int Segments = 16, int Rings = 16);

	void Generate(int Segments, int Rings);
};
