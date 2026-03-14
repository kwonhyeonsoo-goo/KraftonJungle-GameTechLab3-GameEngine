#pragma once
struct FVertexSimple
{
	float x, y, z;    // Position
	float r, g, b, a; // Color
};

static FVertexSimple rect_vertices[] = {
    //   x,      y,      z,      r,     g,     b,     a
    { -0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f }, // 0: 좌측 상단 (Red)
    {  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f }, // 1: 우측 상단 (Green)
    {  0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f }, // 2: 우측 하단 (Blue)
    { -0.5f, -0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  1.0f }  // 3: 좌측 하단 (Yellow)
};

// 시계 방향(CW) 기준
static unsigned int rect_indices[] = {
    0, 1, 2, // 첫 번째 삼각형 (상단-우측-하단)
    0, 2, 3  // 두 번째 삼각형 (상단-하단-좌측)
};