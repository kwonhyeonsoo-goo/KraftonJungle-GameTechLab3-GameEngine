#pragma once
#include <string>

class FFileSystem {
public:
	static std::wstring SearchForSceneFrom(const std::wstring& InStartDirectory);

	static std::wstring FindDefaultScenePath();

	static std::wstring GetAbsolutePath(const std::wstring& path);

	static std::wstring GetParentPath(const std::wstring& path);

	static std::wstring GetExtension(const std::wstring& path);

	static std::wstring JoinPath(const std::wstring& a, const std::wstring& b);

	static std::wstring NormalizePath(const std::wstring& path);

	static std::wstring Utf8ToWide(const std::string& str);

	static std::string WideToUtf8(const std::wstring& wstr);

	static bool IsAbsolutePath(const std::wstring& path);

	static bool HasParentPath(const std::wstring& path);

	static bool SafeExists(const std::wstring& path);

	static std::wstring ToGenericWString(const std::wstring& path);

	static bool GetLineFromFile(FILE* fp, std::string& outLine) {
		char buffer[1024];
		outLine.clear();

		while (fgets(buffer, sizeof(buffer), fp)) {
			outLine += buffer;
			if (outLine.back() == '\n') {
				outLine.pop_back(); // 개행 문자 제거
				if (!outLine.empty() && outLine.back() == '\r') outLine.pop_back(); // Windows 개행 처리
				return true;
			}
		}
		return !outLine.empty();
	}
};