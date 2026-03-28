
#pragma once
#include "Object/Object.h"

struct FNormalVertex
{
	FVector Position;
	FVector Normal;
	FVector4 Color;
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
	std::vector<FNormalVertex> Vertices;
	std::vector<uint32_t> Indices;

	/** .obj 불러올 때 material name 들 저장 **/
	/** Multi-material 고려 (usemtl A, v... / usemtl B, v...) => Section Names A,B **/
	std::vector<std::string> MaterialSlotNames;
	std::vector<SubMeshSection> Sections;
};

class UStaticMesh : public UObject
{

};