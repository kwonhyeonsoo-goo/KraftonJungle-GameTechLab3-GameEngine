
#pragma once
#include "Mesh.h"
class ENGINE_API FStaticMeshRenderData : public FMesh
{
public:
	FStaticMeshRenderData() = default;
	TArray<FString> ImportedTexturePaths;
	TArray<FVector> ImportedDiffuseColors;
	TArray<std::shared_ptr<FMaterial>> DefaultMaterialInstances;
};