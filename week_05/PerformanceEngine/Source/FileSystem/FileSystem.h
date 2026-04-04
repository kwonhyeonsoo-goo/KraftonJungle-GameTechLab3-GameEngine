#pragma once
#include <string>

class FFileSystem {
public:
	static std::wstring SearchForSceneFrom(const std::wstring& InStartDirectory);

	static std::wstring FindDefaultScenePath();
};