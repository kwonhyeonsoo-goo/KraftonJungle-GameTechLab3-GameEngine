#include "PrimitiveGizmo.h"

#include "GizmoMeshGenerator.h"

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
		Generate();
	}
}

void CPrimitiveGizmo::Generate()
{
	auto Data = std::make_shared<FMeshData>();

	TranslationGizmoDesc desc{};
	desc.includeCenterSphere = true;

	auto TranslationGizmo = GenerateTranslationGizmo(desc);
	Mesh Gizmo = Combine(TranslationGizmo);

	Data->Vertices.reserve(Gizmo.vertices.size());
	for (auto& Vertex : Gizmo.vertices)
	{
		Data->Vertices.push_back({ Vertex.position, Vertex.color, Vertex.normal });
	}

	Data->Indices.reserve(Gizmo.indices.size());
	for (auto& Index : Gizmo.indices)
	{
		Data->Indices.push_back(Index);
	}

	MeshData = Data;
	RegisterMeshData(Key, Data);
}
