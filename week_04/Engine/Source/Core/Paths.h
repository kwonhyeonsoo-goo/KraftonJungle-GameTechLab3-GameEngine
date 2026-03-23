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
	static const std::filesystem::path& ProjectRoot();

	// 주요 디렉토리 (모두 절대 경로, 슬래시 끝)
	static std::filesystem::path EngineDir();
	static std::filesystem::path ShaderDir();
	static std::filesystem::path AssetDir();
	static std::filesystem::path SceneDir();
	static std::filesystem::path MaterialDir();
	static std::filesystem::path MeshDir();
	static std::filesystem::path ContentDir();
	static std::filesystem::path ShaderCacheDir();

	/*
	// 경로 결합
	static FString Combine(const FString& Base, const FString& Relative);
	*/

	// FString → std::wstring 변환 (셰이더 로드 등 Win32 API용)
	static std::wstring ToWide(const FString& Path);

private:
	static void SetRoot(const std::filesystem::path& InPath);

	// static FString Root;
	static std::filesystem::path Root;
	static bool bInitialized;
};
