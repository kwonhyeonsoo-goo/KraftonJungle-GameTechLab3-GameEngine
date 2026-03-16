#pragma once

#include "CoreMinimal.h"

struct FMeshData;
class FMaterial;

struct ENGINE_API FRenderCommand
{
	FMeshData* MeshData = nullptr;
	FMatrix WorldMatrix;
	FMaterial* Material = nullptr;
};
