#include "FileSystem.h"
#include <array>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <Windows.h>

std::wstring JoinPath(const std::wstring& a, const std::wstring& b) {
	if (a.empty()) return b;
	if (b.empty()) return a;

	std::wstring result = a;
	// 끝에 슬래시가 없다면 추가
	if (result.back() != L'/' && result.back() != L'\\') {
		result += L'/';
	}
	result += b;
	return result;
}

// 부모 경로를 찾는 수동 함수 (wstring 전용)
std::wstring GetParentPath(const std::wstring& path) {
	size_t found = path.find_last_of(L"/\\");
	if (found == std::wstring::npos) return L"";
	return path.substr(0, found);
}

std::wstring FFileSystem::SearchForSceneFrom(const std::wstring& InStartDirectory)
{
	static const std::array<std::wstring, 2> RelativeCandidates =
	{
		L"PerformanceEngine/Data/Scene/Default.scene",
		L"Data/Scene/Default.scene"
	};

	std::wstring Cursor = InStartDirectory;
	while (!Cursor.empty())
	{
		for (const std::wstring& RelativeCandidate : RelativeCandidates) {
			// 1. 여기서 / 대신 JoinPath 함수를 사용
			const std::wstring Candidate = JoinPath(Cursor, RelativeCandidate);

			// 2. 파일 오픈 시도
			std::ifstream File(Candidate);
			if (File.is_open()) {
				return Candidate; // 찾으면 경로 반환
			}
		}
		// 3. 부모 경로로 이동 (wstring 수동 처리)
		std::wstring Parent = GetParentPath(Cursor);
		if (Parent == Cursor || Parent.empty()) {
			break;
		}
		Cursor = Parent;
	}

	return L"";
}

std::wstring FFileSystem::FindDefaultScenePath()
{
	if (const std::wstring CurrentCandidate = SearchForSceneFrom(std::filesystem::current_path()); !CurrentCandidate.empty())
	{
		return CurrentCandidate;
	}

	std::array<wchar_t, MAX_PATH> ModulePath = {};
	const DWORD CharacterCount = GetModuleFileNameW(nullptr, ModulePath.data(), static_cast<DWORD>(ModulePath.size()));
	if (CharacterCount > 0)
	{
		const std::filesystem::path ModuleDirectory = std::filesystem::path(ModulePath.data()).parent_path();
		return SearchForSceneFrom(ModuleDirectory);
	}

	return {};
}
