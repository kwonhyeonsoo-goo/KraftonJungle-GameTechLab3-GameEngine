#include "PrimitiveCube.h"

const FString CPrimitiveCube::Key = "Cube";
const FString CPrimitiveCube::FilePath = "Assets/Meshes/Cube.mesh";

CPrimitiveCube::CPrimitiveCube()
{
	auto Cached = GetCached(Key);
	if (Cached)
	{
		MeshData = Cached;
	}
	else
	{
		Generate();
	}
}

void CPrimitiveCube::Generate()
{
	auto Data = std::make_shared<FMeshData>();

	FVector4 Red = { 1.0f, 0.3f, 0.3f, 1.0f };
	FVector4 Green = { 0.3f, 1.0f, 0.3f, 1.0f };
	FVector4 Blue = { 0.3f, 0.3f, 1.0f, 1.0f };
	FVector4 Yellow = { 1.0f, 1.0f, 0.3f, 1.0f };
	FVector4 Cyan = { 0.3f, 1.0f, 1.0f, 1.0f };
	FVector4 Magenta = { 1.0f, 0.3f, 1.0f, 1.0f };

	// Front face (z = +0.5) — Red
	Data->Vertices.push_back({ { -0.5f, -0.5f,  0.5f }, Red, {  0.0f,  0.0f,  1.0f } });
	Data->Vertices.push_back({ {  0.5f, -0.5f,  0.5f }, Red, {  0.0f,  0.0f,  1.0f } });
	Data->Vertices.push_back({ {  0.5f,  0.5f,  0.5f }, Red, {  0.0f,  0.0f,  1.0f } });
	Data->Vertices.push_back({ { -0.5f,  0.5f,  0.5f }, Red, {  0.0f,  0.0f,  1.0f } });

	// Back face (z = -0.5) — Green
	Data->Vertices.push_back({ {  0.5f, -0.5f, -0.5f }, Green, {  0.0f,  0.0f, -1.0f } });
	Data->Vertices.push_back({ { -0.5f, -0.5f, -0.5f }, Green, {  0.0f,  0.0f, -1.0f } });
	Data->Vertices.push_back({ { -0.5f,  0.5f, -0.5f }, Green, {  0.0f,  0.0f, -1.0f } });
	Data->Vertices.push_back({ {  0.5f,  0.5f, -0.5f }, Green, {  0.0f,  0.0f, -1.0f } });

	// Top face (y = +0.5) — Blue
	Data->Vertices.push_back({ { -0.5f,  0.5f,  0.5f }, Blue, {  0.0f,  1.0f,  0.0f } });
	Data->Vertices.push_back({ {  0.5f,  0.5f,  0.5f }, Blue, {  0.0f,  1.0f,  0.0f } });
	Data->Vertices.push_back({ {  0.5f,  0.5f, -0.5f }, Blue, {  0.0f,  1.0f,  0.0f } });
	Data->Vertices.push_back({ { -0.5f,  0.5f, -0.5f }, Blue, {  0.0f,  1.0f,  0.0f } });

	// Bottom face (y = -0.5) — Yellow
	Data->Vertices.push_back({ { -0.5f, -0.5f, -0.5f }, Yellow, {  0.0f, -1.0f,  0.0f } });
	Data->Vertices.push_back({ {  0.5f, -0.5f, -0.5f }, Yellow, {  0.0f, -1.0f,  0.0f } });
	Data->Vertices.push_back({ {  0.5f, -0.5f,  0.5f }, Yellow, {  0.0f, -1.0f,  0.0f } });
	Data->Vertices.push_back({ { -0.5f, -0.5f,  0.5f }, Yellow, {  0.0f, -1.0f,  0.0f } });

	// Right face (x = +0.5) — Cyan
	Data->Vertices.push_back({ {  0.5f, -0.5f,  0.5f }, Cyan, {  1.0f,  0.0f,  0.0f } });
	Data->Vertices.push_back({ {  0.5f, -0.5f, -0.5f }, Cyan, {  1.0f,  0.0f,  0.0f } });
	Data->Vertices.push_back({ {  0.5f,  0.5f, -0.5f }, Cyan, {  1.0f,  0.0f,  0.0f } });
	Data->Vertices.push_back({ {  0.5f,  0.5f,  0.5f }, Cyan, {  1.0f,  0.0f,  0.0f } });

	// Left face (x = -0.5) — Magenta
	Data->Vertices.push_back({ { -0.5f, -0.5f, -0.5f }, Magenta, { -1.0f,  0.0f,  0.0f } });
	Data->Vertices.push_back({ { -0.5f, -0.5f,  0.5f }, Magenta, { -1.0f,  0.0f,  0.0f } });
	Data->Vertices.push_back({ { -0.5f,  0.5f,  0.5f }, Magenta, { -1.0f,  0.0f,  0.0f } });
	Data->Vertices.push_back({ { -0.5f,  0.5f, -0.5f }, Magenta, { -1.0f,  0.0f,  0.0f } });

	// 36 indices (6 faces * 2 triangles * 3 vertices)
	for (unsigned int i = 0; i < 6; ++i)
	{
		unsigned int Base = i * 4;
		Data->Indices.push_back(Base + 0);
		Data->Indices.push_back(Base + 1);
		Data->Indices.push_back(Base + 2);
		Data->Indices.push_back(Base + 0);
		Data->Indices.push_back(Base + 2);
		Data->Indices.push_back(Base + 3);
	}

	MeshData = Data;
	RegisterMeshData(Key, Data);
}
