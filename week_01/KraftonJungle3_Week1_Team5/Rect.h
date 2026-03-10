#pragma once
struct FVertexColor
{
	float x, y, z;
	float r, g, b, a;
};

FVertexColor rect_vertices[] = {
	{ -0.5f,  0.5f, 0.0f , 1.f, 0.f, 0.f, 1.f }, // Top left vertex
	{  0.5f,  0.5f, 0.0f , 1.f, 0.f, 0.f, 1.f }, // Top right vertex
	{  0.5f, -0.5f, 0.0f , 1.f, 0.f, 0.f, 1.f }, // Bottom Right vertex
	{  0.5f, -0.5f, 0.0f , 1.f, 0.f, 0.f, 1.f }, // Bottom Right vertex
	{ -0.5f, -0.5f, 0.0f , 1.f, 0.f, 0.f, 1.f }, // Bottom left vertex
	{ -0.5f,  0.5f, 0.0f , 1.f, 0.f, 0.f, 1.f }, // Top left vertex
};
