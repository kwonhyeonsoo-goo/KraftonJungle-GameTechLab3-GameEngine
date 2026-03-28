#include "StaticMesh.h"
#include "Object/Class.h"
#include "Renderer/Mesh/StaticMeshRenderData.h"

IMPLEMENT_RTTI(UStaticMesh, UObject)

const FString& UStaticMesh::GetAssetPath() const
{
	return MeshAsset->Path;
}
