#pragma once
#include "CoreMinimal.h"

// .mtl 파일에서 파싱한 머티리얼 정보
struct FObjMaterialInfo
{
	FString Name;
	FVector DiffuseColor;       // Kd
	FString DiffuseTexturePath; // map_Kd
	// 필요하면 Specular, Normal 등 추가
};

// .obj 파일에서 파싱한 Raw 데이터
struct FObjInfo
{
	FString ObjDirectory;
	// 위치/UV/노말 — 인덱스 풀에서 참조됨
	TArray<FVector>  Positions;   // v
	TArray<FVector2> UVs;         // vt
	TArray<FVector>  Normals;     // vn

	// 면 인덱스 (v/vt/vn 각각)
	TArray<uint32> PositionIndices;
	TArray<uint32> UVIndices;
	TArray<uint32> NormalIndices;

	// usemtl 단위 섹션 경계
	TArray<FString> MaterialNames;          // 섹션별 머티리얼 이름
	TArray<uint32>  SectionStartIndices;    // 각 섹션 시작 인덱스 오프셋

	// .mtl에서 파싱한 머티리얼 정보들
	TArray<FObjMaterialInfo> Materials;
};
