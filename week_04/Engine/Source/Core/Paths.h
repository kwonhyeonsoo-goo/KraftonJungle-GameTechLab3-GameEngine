#pragma once

#include "CoreMinimal.h"
#include <string>
#include <filesystem>

class ENGINE_API FPaths
{
public:
	// 실행 파일 위치 기준으로 프로젝트 루트를 자동 탐색 (KraftonJungleEngine.sln 기준)
	static void Initialize();

	// 프로젝트 루트 (슬래시 끝)
	static const FString& ProjectRoot();

	// 주요 디렉토리 (모두 절대 경로, 슬래시 끝)
	static FString EngineDir();
	static FString ShaderDir();
	static FString AssetDir();
	static FString SceneDir();
	static FString MaterialDir();
	static FString MeshDir();
	static FString ContentDir();
	static FString ShaderCacheDir();

	// 경로 결합
	static FString Combine(const FString& Base, const FString& Relative);

	// FString → std::wstring 변환 (셰이더 로드 등 Win32 API용)
	static std::wstring ToWide(const FString& Path);

private:
	static void SetRoot(const std::filesystem::path& InPath);

	static FString Root;
	static bool bInitialized;
};
