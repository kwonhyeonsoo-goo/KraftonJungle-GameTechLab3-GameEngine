#pragma once

#include "CoreMinimal.h"

struct FMeshData;

struct ENGINE_API FRenderCommand
{
	FMeshData* MeshData = nullptr;
	FMatrix WorldMatrix;
};
