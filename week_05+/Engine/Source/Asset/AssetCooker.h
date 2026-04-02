#pragma once
#include "CoreMinimal.h"
#include "EngineAPI.h"

class FStaticMeshRenderData;

class ENGINE_API FAssetCooker
{
public:
	// 소스가 변경되어 재쿡이 필요한지 판단
	static bool NeedsCook(const FString& SourcePath, const FString& CookedPath);

	// OBJ → .dasset 바이너리 쿡 (기존 파이프라인으로 파싱 후 바이너리 직렬화)
	static bool CookStaticMesh(const FString& ObjSourcePath, const FString& OutputPath);

	// .dasset 바이너리에서 FStaticMeshRenderData 복원 (파싱 없이 읽기만)
	static FStaticMeshRenderData* LoadCookedStaticMesh(const FString& CookedPath);

	// 소스 상대 경로 → 쿡된 파일 경로 변환
	// 예: "Assets/Meshes/Cat.obj" → "{ProjectRoot}/Intermediate/Cooked/Assets/Meshes/Cat.dasset"
	static FString GetCookedPath(const FString& SourceRelativePath);
	static bool SaveCookedStaticMesh(const FStaticMeshRenderData* RenderData,
		const FString& SourcePath,
		const FString& OutputPath);
private:
	// 파일 내용 전체를 읽어 FNV-1a 64bit 해시
	static uint64 ComputeFileHash(const FString& AbsolutePath);

	// 파일 최종 수정 시간 (file_time_type → uint64)
	static uint64 GetFileTimestamp(const FString& AbsolutePath);

	// Intermediate/Cooked/ 하위 디렉토리 자동 생성
	static void EnsureDirectoryExists(const FString& FilePath);
};
