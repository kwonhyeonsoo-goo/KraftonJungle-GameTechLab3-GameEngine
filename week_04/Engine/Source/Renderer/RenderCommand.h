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

struct ENGINE_API FTextRenderCommand
{
	FString Text;
	FVector WorldPosition;
	float WorldScale = 0.3f;
	FVector4 Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
};

struct ENGINE_API FSubUVRenderCommand
{
	FVector WorldPosition;
	FVector2 Size = FVector2(0.3f, 0.3f);
	FVector4 Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

	int32 Columns = 9;
	int32 Rows = 4;
	int32 TotalFrames = 36;

	uint32 FirstFrame = 14;
	uint32 LastFrame = 17;
	
	float FPS = 8.0f;
	float ElapsedTime = 0.0f;

	bool bLoop = true;
	bool bBillboard = true;

};

// Scene → Renderer 간 전달되는 프레임 단위 커맨드 큐
// Scene은 Renderer를 몰라도 이 큐에 데이터를 쌓을 수 있음
struct ENGINE_API FRenderCommandQueue
{
	TArray<FRenderCommand> Commands;

	TArray<FTextRenderCommand> TextCommands;

	TArray<FSubUVRenderCommand> SubUVCommands;

	FMatrix ViewMatrix;
	FMatrix ProjectionMatrix;

	void Reserve(size_t Count)
	{
		Commands.reserve(Count);

		TextCommands.reserve(Count);

		SubUVCommands.reserve(Count);
	}

	void AddCommand(const FRenderCommand& Cmd)
	{
		Commands.push_back(Cmd);
	}

	void AddTextCommand(const FTextRenderCommand& Cmd)
	{
		TextCommands.push_back(Cmd);
	}

	void AddSubUVCommand(const FSubUVRenderCommand& Cmd)
	{
		SubUVCommands.push_back(Cmd);
	}

	void Clear()
	{
		Commands.clear();
	}
};
