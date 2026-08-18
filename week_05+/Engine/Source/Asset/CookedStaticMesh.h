#pragma once
#include "CoreMinimal.h"
struct FCookedStaticMeshData
{
	// --- Mesh 기본 정보 ---
	uint32 VertexCount;
	uint32 IndexCount;
	uint32 SectionCount;
	FVector MinBound;
	FVector MaxBound;
	float BoundRadius;

	// --- 가변 길이 데이터 (이후 연속 기록) ---
	// FPrimitiveVertex Vertices[VertexCount]
	// uint32 Indices[IndexCount]
	// FCookedMeshSection Sections[SectionCount]
	// 각 Section의 텍스처 경로 (null-terminated string)
	// 각 Section의 Diffuse 색상 (FVector)
};

struct FCookedMeshSection
{
	uint32 FirstIndex;
	uint32 IndexCount;
	uint32 MaterialIndex;
	uint32 TexturePathLength;  // 텍스처 경로 문자열 길이
};