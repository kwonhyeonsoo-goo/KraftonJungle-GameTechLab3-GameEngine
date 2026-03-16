#include "PrimitiveGizmo.h"

#include "UnrealEditorStyledGizmo.h"

const FString CPrimitiveGizmo::Key = "Gizmo";
const FString CPrimitiveGizmo::FilePath = "Assets/Meshed/Gizmo.mesh";

CPrimitiveGizmo::CPrimitiveGizmo()
{
	auto Cached = GetCached(Key);
	if (Cached)
	{
		MeshData = Cached;
	}
	else
	{
		Generate(EGizmoType::Scale);
	}
}

void CPrimitiveGizmo::Generate(EGizmoType Type)
{
	if (Type == EGizmoType::Translation)
	{
		GenerateTranslationGizmoMesh();
	}

	if (Type == EGizmoType::Rotation)
	{
		GenerateRotationGizmoMesh();
	}

	if (Type == EGizmoType::Scale)
	{
		GenerateScaleGizmoMesh();
	}
}

void CPrimitiveGizmo::GenerateTranslationGizmoMesh()
{
	auto Data = std::make_shared<FMeshData>();

	TranslationDesc desc{};

	auto TranslationGizmo = GenerateTranslationGizmo(desc);
	Mesh Gizmo = Combine(TranslationGizmo);

	Data->Vertices.reserve(Gizmo.vertices.size());
	for (auto& Vertex : Gizmo.vertices)
	{
		FVector4 Color = { Vertex.color.r, Vertex.color.g, Vertex.color.b, Vertex.color.a };
		Data->Vertices.push_back({ Vertex.position, Color, Vertex.normal });
	}

	Data->Indices.reserve(Gizmo.indices.size());
	for (auto& Index : Gizmo.indices)
	{
		Data->Indices.push_back(Index);
	}

	MeshData = Data;
	RegisterMeshData(Key, Data);
}

void CPrimitiveGizmo::GenerateRotationGizmoMesh()
{
	auto Data = std::make_shared<FMeshData>();

	RotationDesc desc{};

	auto RotationGizmo = GenerateRotationGizmo(desc);
	Mesh Gizmo = Combine(RotationGizmo);

	Data->Vertices.reserve(Gizmo.vertices.size());
	for (auto& Vertex : Gizmo.vertices)
	{
		FVector4 Color = { Vertex.color.r, Vertex.color.g, Vertex.color.b, Vertex.color.a };
		Data->Vertices.push_back({ Vertex.position, Color, Vertex.normal });
	}

	Data->Indices.reserve(Gizmo.indices.size());
	for (auto& Index : Gizmo.indices)
	{
		Data->Indices.push_back(Index);
	}

	MeshData = Data;
	RegisterMeshData(Key, Data);
}

void CPrimitiveGizmo::GenerateScaleGizmoMesh()
{
	auto Data = std::make_shared<FMeshData>();

	ScaleDesc desc{};

	auto ScaleGizmo = GenerateScaleGizmo(desc);
	Mesh Gizmo = Combine(ScaleGizmo);

	Data->Vertices.reserve(Gizmo.vertices.size());
	for (auto& Vertex : Gizmo.vertices)
	{
		FVector4 Color = { Vertex.color.r, Vertex.color.g, Vertex.color.b, Vertex.color.a };
		Data->Vertices.push_back({ Vertex.position, Color, Vertex.normal });
	}

	Data->Indices.reserve(Gizmo.indices.size());
	for (auto& Index : Gizmo.indices)
	{
		Data->Indices.push_back(Index);
	}

	MeshData = Data;
	RegisterMeshData(Key, Data);
}

