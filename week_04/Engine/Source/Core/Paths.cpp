#include "Paths.h"
#include <filesystem>
#include <Windows.h>

namespace fs = std::filesystem;

FString FPaths::Root;
bool FPaths::bInitialized = false;

void FPaths::Initialize()
{
	if (bInitialized)
	{
		return;
	}

	// 실행 파일 경로 획득
	wchar_t ExePath[MAX_PATH] = {};
	GetModuleFileNameW(nullptr, ExePath, MAX_PATH);

	fs::path ExeDir = fs::path(ExePath).parent_path();

	// 실행 파일은 항상 {Root}/{Project}/Bin/{Config}/ 에 위치
	// 3단계 상위 = 프로젝트 루트
	fs::path Candidate = ExeDir.parent_path().parent_path().parent_path();

	// 검증: Engine/ 과 Assets/ 디렉토리가 모두 존재하는지 확인
	if (fs::is_directory(Candidate / "Engine") && fs::is_directory(Candidate / "Assets"))
	{
		SetRoot(Candidate);
		return;
	}

	// 폴백: 상위 디렉토리를 순회하며 Engine/ + Assets/ 조합 탐색
	fs::path CurrentDir = ExeDir;
	for (int32 i = 0; i < 10; ++i)
	{
		if (fs::is_directory(CurrentDir / "Engine") && fs::is_directory(CurrentDir / "Assets"))
		{
			SetRoot(CurrentDir);
			return;
		}
		CurrentDir = CurrentDir.parent_path();
	}

	// 최종 폴백: 실행 파일 디렉토리 사용
	SetRoot(ExeDir);
}

void FPaths::SetRoot(const std::filesystem::path& InPath)
{
	Root = InPath.string();
	for (auto& Ch : Root)
	{
		if (Ch == '\\')
		{
			Ch = '/';
		}
	}
	if (!Root.empty() && Root.back() != '/')
	{
		Root += '/';
	}
	bInitialized = true;
}

const FString& FPaths::ProjectRoot()
{
	return Root;
}

FString FPaths::EngineDir()
{
	return Root + "Engine/";
}

FString FPaths::ShaderDir()
{
	return Root + "Engine/Shaders/";
}

FString FPaths::AssetDir()
{
	return Root + "Assets/";
}

FString FPaths::SceneDir()
{
	return Root + "Assets/Scenes/";
}

FString FPaths::MaterialDir()
{
	return Root + "Assets/Materials/";
}

FString FPaths::MeshDir()
{
	return Root + "Assets/Meshes/";
}

FString FPaths::ContentDir()
{
	return Root + "Content/";
}

FString FPaths::ShaderCacheDir()
{
	return Root + "Content/Shaders/";
}

FString FPaths::Combine(const FString& Base, const FString& Relative)
{
	if (Base.empty())
	{
		return Relative;
	}
	if (Base.back() == '/' || Base.back() == '\\')
	{
		return Base + Relative;
	}
	return Base + "/" + Relative;
}

std::wstring FPaths::ToWide(const FString& Path)
{
	return std::wstring(Path.begin(), Path.end());
}
