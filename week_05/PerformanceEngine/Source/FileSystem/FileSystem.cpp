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

std::wstring FFileSystem::GetAbsolutePath(const std::wstring& path)
{
	if (path.empty()) return L"";

	// 1. 이미 절대 경로라면 정규화만 해서 반환
	if (IsAbsolutePath(path)) {
		return NormalizePath(path);
	}

	// 2. Windows API를 이용해 전체 경로 계산 (가장 안전함)
	// 먼저 필요한 버퍼 크기를 알아냅니다.
	DWORD requiredSize = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
	if (requiredSize == 0) return path; // 실패 시 입력값 그대로 반환

	std::wstring fullPath(requiredSize, L'\0');
	if (GetFullPathNameW(path.c_str(), requiredSize, &fullPath[0], nullptr) == 0) {
		return path;
	}

	// 널 문자가 포함되어 있을 수 있으므로 실제 길이에 맞춰 자르기
	fullPath.resize(wcslen(fullPath.c_str()));

	// 3. 마지막으로 우리 엔진 표준(Normalize)에 맞춰 슬래시 정리
	return NormalizePath(fullPath);
}

std::wstring FFileSystem::GetParentPath(const std::wstring& path)
{
	size_t found = path.find_last_of(L"/\\");
	if (found == std::wstring::npos) return L"";
	return path.substr(0, found);
}

std::wstring FFileSystem::GetExtension(const std::wstring& path)
{
	if (path.empty()) return L"";

	// 1. 뒤에서부터 점('.')의 위치를 찾습니다.
	size_t dotPos = path.find_last_of(L".");

	// 2. 점이 없거나, 점이 문자열의 마지막인 경우 (예: "file.")
	if (dotPos == std::wstring::npos || dotPos == path.length() - 1) {
		return L"";
	}

	// 3. 점이 경로 구분자보다 앞에 있으면 안 됩니다. (예: "dir.name/file")
	size_t lastSeparator = path.find_last_of(L"/\\");
	if (lastSeparator != std::wstring::npos && dotPos < lastSeparator) {
		return L"";
	}

	// 4. 점을 포함한 확장자 반환 (예: ".scene")
	return path.substr(dotPos);
}

std::wstring FFileSystem::JoinPath(const std::wstring& a, const std::wstring& b)
{
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

std::wstring FFileSystem::NormalizePath(const std::wstring& path)
{
	if (path.empty()) return L"";

	// 1. 모든 슬래시를 하나로 통일 (윈도우/유닉스 혼용 방지)
	std::wstring unifiedPath = path;
	for (auto& c : unifiedPath) {
		if (c == L'\\') c = L'/';
	}

	// 2. '/' 기준으로 분리
	std::vector<std::wstring> parts;
	std::wstring part;
	std::wstringstream ss(unifiedPath);
	while (std::getline(ss, part, L'/')) {
		if (part.empty() || part == L".") continue; // 현재 폴더(.)는 무시
		if (part == L"..") {
			if (!parts.empty() && parts.back() != L"..") {
				parts.pop_back(); // 이전 폴더(..)면 스택에서 하나 제거
			}
			else {
				// 루트를 넘어서는 .. 인 경우 (상대 경로일 때만 유지)
				parts.push_back(L"..");
			}
			continue;
		}
		parts.push_back(part);
	}

	// 3. 다시 합치기
	std::wstring result;
	// 맨 앞이 슬래시였다면(절대경로 등) 복구
	if (path[0] == L'/' || path[0] == L'\\') result += L'/';

	for (size_t i = 0; i < parts.size(); ++i) {
		result += parts[i];
		if (i != parts.size() - 1) result += L'/';
	}

	return result;
}

std::wstring FFileSystem::Utf8ToWide(const std::string& str)
{
	if (str.empty()) return L"";
	int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
	std::wstring wstr(size, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size);
	return wstr;
}

std::string FFileSystem::WideToUtf8(const std::wstring& wstr)
{
	if (wstr.empty()) return "";
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

bool FFileSystem::IsAbsolutePath(const std::wstring& path)
{
	if (path.empty()) return false;

	// 1. 드라이브 문자 방식 (예: C:\ 또는 D:/)
	// 두 번째 글자가 ':' 이고 세 번째 글자가 슬래시인 경우
	if (path.length() >= 3) {
		wchar_t drive = towupper(path[0]);
		if (drive >= L'A' && drive <= L'Z' && path[1] == L':') {
			if (path[2] == L'\\' || path[2] == L'/') {
				return true;
			}
		}
	}

	// 2. 네트워크 경로(UNC) 방식 (예: \\Server\Share)
	if (path.length() >= 2) {
		if ((path[0] == L'\\' && path[1] == L'\\') || (path[0] == L'/' && path[1] == L'/')) {
			return true;
		}
	}

	// 3. 리눅스 스타일 또는 유닉스 스타일 (예: /usr/bin) - 윈도우에서도 간혹 사용
	if (path[0] == L'/' || path[0] == L'\\') {
		// 드라이브 문자가 없더라도 루트부터 시작하면 절대 경로로 취급하는 경우가 있음
		return true;
	}

	return false;
}

bool FFileSystem::HasParentPath(const std::wstring& path)
{
	if (path.empty()) return false;

	// 경로에서 마지막 슬래시의 위치를 찾습니다.
	size_t found = path.find_last_of(L"/\\");

	// 1. 슬래시가 전혀 없으면 부모가 없는 것 (파일명만 있는 상태)
	if (found == std::wstring::npos) return false;

	// 2. 슬래시가 맨 앞에 하나만 있는 경우 (예: "/test" -> 루트가 부모임)
	// 3. 드라이브 문자 직후에 슬래시가 있는 경우 (예: "C:/test")
	// 이 경우들은 부모가 있다고 판단하는 것이 일반적입니다.

	// 단순하게 슬래시가 발견되면 일단 부모 경로로 올라갈 여지가 있다고 봅니다.
	return true;
}

bool FFileSystem::SafeExists(const std::wstring& path)
{
	// GetFileAttributesW는 인코딩 변환 시도를 하지 않고 
	// 파일 시스템에 해당 경로가 있는지 직접 물어봅니다.
	DWORD dwAttrib = GetFileAttributesW(path.c_str());

	return (dwAttrib != INVALID_FILE_ATTRIBUTES);
}

std::wstring FFileSystem::ToGenericWString(const std::wstring& path)
{
	std::wstring result = path;
	for (auto& c : result) {
		if (c == L'\\') c = L'/';
	}
	return result;
}
