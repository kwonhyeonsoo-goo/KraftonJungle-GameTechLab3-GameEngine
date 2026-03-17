#pragma once

#include "CoreMinimal.h"

struct FMeshData;
class FMaterial;

struct ENGINE_API FRenderCommand
{
	FMeshData* MeshData = nullptr;
	FMatrix WorldMatrix;
	FMaterial* Material = nullptr;
	uint64 SortKey = 0;
	bool bOverlay = false;
	bool bDisableDepthTest = false;
	bool bDisableDepthWrite = false;
	bool bDisableCulling = false;

	static uint64 MakeSortKey(const FMaterial* InMaterial, const FMeshData* InMeshData);
};

// Scene → Renderer 간 전달되는 프레임 단위 커맨드 큐
// Scene은 Renderer를 몰라도 이 큐에 데이터를 쌓을 수 있음
struct ENGINE_API FRenderCommandQueue
{
	TArray<FRenderCommand> Commands;
	FMatrix ViewMatrix;
	FMatrix ProjectionMatrix;

	void Reserve(size_t Count)
	{
		Commands.reserve(Count);
	}

	void AddCommand(const FRenderCommand& Cmd)
	{
		Commands.push_back(Cmd);
	}

	void Clear()
	{
		Commands.clear();
	}
};
