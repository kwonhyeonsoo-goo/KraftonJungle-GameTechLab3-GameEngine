#pragma once

#include <filesystem>

struct FStaticMeshSourceData;

class FObjParser
{
public:
	static bool Parse(const std::wstring& InObjPath, FStaticMeshSourceData& OutSourceData);

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
