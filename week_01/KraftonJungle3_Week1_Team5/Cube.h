#pragma once

struct FVertexColor
{
	float x, y, z;
	float r, g, b, a;
};

FVertexColor cube_vertices[] =
{
	{ -1.0f,  1.0f, 0.0f , 1.f, 0.f, 0.f, 1.f }, // Top left vertex
	{  1.0f,  1.0f, 0.0f , 1.f, 0.f, 0.f, 1.f }, // Top right vertex
	{  1.0f, -1.0f, 0.0f , 1.f, 0.f, 0.f, 1.f }, // Bottom Right vertex
	{  1.0f, -1.0f, 0.0f , 1.f, 0.f, 0.f, 1.f }, // Bottom Right vertex
	{ -1.0f, -1.0f, 0.0f , 1.f, 0.f, 0.f, 1.f }, // Bottom left vertex
	{ -1.0f,  1.0f, 0.0f , 1.f, 0.f, 0.f, 1.f }, // Top left vertex
};