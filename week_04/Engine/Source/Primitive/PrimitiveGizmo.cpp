#include "PrimitiveGizmo.h"

#include "UnrealEditorStyledGizmo.h"

namespace UEStyledGizmo = codex::gizmo::unreal_editor;

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

	UEStyledGizmo::TranslationDesc desc{};

	auto TranslationGizmo = UEStyledGizmo::GenerateTranslationGizmo(desc);
	UEStyledGizmo::Mesh Gizmo = UEStyledGizmo::Combine(TranslationGizmo);

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

	UEStyledGizmo::RotationDesc desc{};

	auto RotationGizmo = UEStyledGizmo::GenerateRotationGizmo(desc);
	UEStyledGizmo::Mesh Gizmo = UEStyledGizmo::Combine(RotationGizmo);

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

	UEStyledGizmo::ScaleDesc desc{};

	auto ScaleGizmo = UEStyledGizmo::GenerateScaleGizmo(desc);
	UEStyledGizmo::Mesh Gizmo = UEStyledGizmo::Combine(ScaleGizmo);

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

