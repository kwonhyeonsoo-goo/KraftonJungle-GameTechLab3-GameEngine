#pragma once
#include "CoreMinimal.h"
#include <d3d11.h>

struct ENGINE_API FFontGlyph
{
	float u0 = 0.0f;
	float v0 = 0.0f;
	float u1 = 0.0f;
	float v1 = 0.0f;

	float width = 1.0f;
	float height = 1.0f;
	float Advance = 1.0f;
};

//class ENGINE_API FFontAtlas
//{
//public:
//	FFontAtlas() = default;
//	~FFontAtlas();
//
