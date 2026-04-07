#include "RenderCommand.h"
#include "Material.h"
#include "Primitive/PrimitiveBase.h"

uint64 FRenderCommand::MakeSortKey(const FMaterial* InMaterial, const FMeshData* InMeshData, uint32 InFirstIndex)
{
	uint32 MatId = InMaterial ? static_cast<uint32>(InMaterial->GetSortId()) : 0;
	uint32 MeshId = InMeshData ? InMeshData->GetSortId() : 0;
	return (static_cast<uint64>(MatId) << 32) | (static_cast<uint64>(MeshId) << 16) | (InFirstIndex & 0xFFFF);
}