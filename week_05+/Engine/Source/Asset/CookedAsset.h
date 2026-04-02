#pragma once
#include "CoreMinimal.h"
#include "EngineAPI.h"

// ─── 매직 넘버 & 버전 ───
constexpr uint32 DINO_COOKED_MAGIC = 0x44494E4F; // 'DINO'

// 에셋 타입별 쿡 포맷 버전 — 구조가 바뀔 때마다 올린다
constexpr uint32 COOKED_STATIC_MESH_VERSION = 1;
constexpr uint32 COOKED_TEXTURE_VERSION = 1;

enum class ECookedAssetType : uint32
{
	StaticMesh = 1,
	Texture = 2,
	Material = 3,
};

// 쿡된 파일 헤더 (파일 맨 앞에 고정 크기로 기록)
struct FCookedAssetHeader
{
	uint32 Magic;              // DINO_COOKED_MAGIC
	uint32 Version;            // 쿡 포맷 버전
	uint32 AssetType;          // ECookedAssetType
	uint64 SourceHash;         // 소스 파일 내용 해시 (변경 감지)
	uint64 SourceTimestamp;    // 소스 파일 최종 수정 시간 (빠른 dirty check)
	uint32 DataOffset;         // 실제 데이터 시작 오프셋 (보통 sizeof(FCookedAssetHeader))
	uint32 DataSize;           // 데이터 바이트 수
	uint32 Reserved[4];        // 나중에 필드 추가할 때 쓸 여유 공간
};

// Section 직렬화용 구조체 (가변 길이 문자열 제외, 고정 크기)
struct FCookedMeshSection
{
	uint32 FirstIndex;
	uint32 IndexCount;
	uint32 MaterialIndex;
	uint32 TexturePathLength;  // 바이트 수 (null 미포함)
};
