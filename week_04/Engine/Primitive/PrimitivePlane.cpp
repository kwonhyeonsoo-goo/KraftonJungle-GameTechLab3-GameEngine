#include "PrimitivePlane.h"

const FString CPrimitivePlane::Key = "Plane";
const FString CPrimitivePlane::FilePath = "Assets/Meshes/Plane.mesh";

CPrimitivePlane::CPrimitivePlane()
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

void CPrimitivePlane::Generate()
{
	auto Data = std::make_shared<FMeshData>();

	FVector4 White = { 1.0f, 1.0f, 1.0f, 1.0f };
	FVector Normal = { 0.0f, 1.0f, 0.0f };

	Data->Vertices.push_back({ { -0.5f, 0.0f,  0.5f }, White, Normal });
	Data->Vertices.push_back({ {  0.5f, 0.0f,  0.5f }, White, Normal });
	Data->Vertices.push_back({ {  0.5f, 0.0f, -0.5f }, White, Normal });
	Data->Vertices.push_back({ { -0.5f, 0.0f, -0.5f }, White, Normal });

	Data->Indices.push_back(0);
	Data->Indices.push_back(1);
	Data->Indices.push_back(2);
	Data->Indices.push_back(0);
	Data->Indices.push_back(2);
	Data->Indices.push_back(3);

	MeshData = Data;
	RegisterMeshData(Key, Data);
}
