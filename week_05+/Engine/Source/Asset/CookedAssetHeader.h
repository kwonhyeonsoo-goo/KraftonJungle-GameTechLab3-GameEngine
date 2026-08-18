#pragma once
#include "Types/CoreTypes.h"

// 모든 .dasset 파일의 공통 헤더
struct FCookedAssetHeader
{
	uint32 Magic;           // 'DINO' = 0x44494E4F
	uint32 Version;         // 쿡 포맷 버전 (올릴 때마다 ++). 이걸로 구버전 감지
	uint32 AssetType;       // EAssetType enum (StaticMesh, Texture, Material...)
	uint64 SourceHash;      // 소스 파일 해시 (변경 감지용)
	uint64 SourceTimestamp;  // 소스 파일 최종 수정 시간 (빠른 dirty check)
	uint32 DataOffset;      // 실제 데이터 시작 위치
	uint32 DataSize;        // 데이터 크기
};

enum class ECookedAssetType : uint32
{
	StaticMesh = 1,
	Texture = 2,
	Material = 3,
};

// 쿡 포맷 버전 상수
constexpr uint32 COOKED_STATIC_MESH_VERSION = 1;
constexpr uint32 COOKED_TEXTURE_VERSION = 1;