#pragma once

#pragma once
#include "Object/Object.h"

// FPrimitiveVertex와 메모리 레이아웃 일치: Position, Color, Normal, UV
struct FNormalVertex
{
	FVector Position;
	FVector4 Color;
	FVector Normal;
	FVector2 UV;
};

struct FMeshSection
{
	FString MaterialName;
	TArray<uint32> Indices;
};

struct SubMeshSection
{
	int32 IndexStart;
	int32 IndexCount;
	int32 MaterialIndex;
};

// Cooked GPU Data
struct FStaticMesh
{
	std::string Path;
	std::string MtlPath; // MTL 파일 전체 경로
	std::vector<FNormalVertex> Vertices;
	std::vector<uint32_t> Indices;

	/** .obj 불러올 때 material name 들 저장 **/
	/** Multi-material 고려 (usemtl A, v... / usemtl B, v...) => Section Names A,B **/
	std::vector<std::string> MaterialSlotNames; //Dorumon_body, Dorumon_eye
	std::vector<SubMeshSection> Sections; //Material별로 섹션을 나누기 위함
	TArray<class FMaterial*> Materials;
};