#pragma once

#include <filesystem>

struct FStaticMeshSourceData;

class FObjParser
{
public:
	static bool Parse(const std::filesystem::path& InObjPath, FStaticMeshSourceData& OutSourceData);
};
