#include "Mesh.h"

const TArray<FMeshSection>& FMesh::GetSections() const
{
	if (MeshData) 
		return MeshData->Sections;
	// TODO: insert return statement here
	static TArray<FMeshSection> Empty;
	return Empty;
}

FMaterial* FMesh::GetDefaultMaterial(uint32 SlotIndex) const
{
	if(SlotIndex < DefaultMaterials.size())
		return DefaultMaterials[SlotIndex];
    return nullptr;
}

void FMesh::SetDefaultMaterial(uint32 SlotIndex, FMaterial* Mat)
{
	if (SlotIndex >= DefaultMaterials.size())
		DefaultMaterials.resize(SlotIndex + 1, nullptr);
	DefaultMaterials[SlotIndex] = Mat;
}
