#pragma once

struct FMatrix4
{
	float M[4][4];

	FMatrix4() : M{
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1}
	} {
	}

	static const FMatrix4 Identity()
	{
		return FMatrix4();
	}
};