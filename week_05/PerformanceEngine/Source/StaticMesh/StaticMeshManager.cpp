#include "StaticMesh/StaticMeshManager.h"

#include <algorithm>
#include <cctype>
#include <string>

#include "StaticMesh/ObjParser.h"
#include "StaticMesh/StaticMesh.h"
#include "FileSystem/FileSystem.h"
namespace
{
	std::string ToLowerCopy(std::string InValue)
	{
		std::ranges::transform(InValue
		                       ,
		                       InValue.begin(),
		                       [](unsigned char Ch) { return static_cast<char>(std::tolower(Ch)); });
		return InValue;
	}
}

FString FStaticMeshManager::BuildAssetKey(const std::wstring& InAssetPath)
{
	if (InAssetPath.empty())
	{
		return {};
	}

	std::wstring NormalPath = FFileSystem::NormalizePath(FFileSystem::GetAbsolutePath(InAssetPath));
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, NormalPath.c_str(), (int)NormalPath.length(), NULL, 0, NULL, NULL);

	std::string utf8Str(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, NormalPath.c_str(), (int)NormalPath.length(), &utf8Str[0], size_needed, NULL, NULL);

	return FString(utf8Str.c_str());
}

std::shared_ptr<FStaticMesh> FStaticMeshManager::LoadStaticMesh(
	ID3D11Device* InDevice,
	ID3D11DeviceContext* InDeviceContext,
	const std::wstring& InAssetPath)
{
	if (InDevice == nullptr || InDeviceContext == nullptr || InAssetPath.empty())
	{
		return {};
	}

	const std::wstring NormalizedPath = FFileSystem::NormalizePath(FFileSystem::GetAbsolutePath(InAssetPath));
	const FString MeshCacheKey = BuildAssetKey(NormalizedPath);

	const auto ExistingMeshIt = MeshCache.find(MeshCacheKey);
	if (ExistingMeshIt != MeshCache.end())
	{
		return ExistingMeshIt->second;
	}

	const std::wstring ExtensionW = FFileSystem::GetExtension(NormalizedPath);
	/*const std::string Extension = ToLowerCopy(NormalizedPath.extension().string());*/

	FStaticMeshSourceData SourceData;
	if (ExtensionW == L".obj")
	{
		if (!FObjParser::Parse(NormalizedPath, SourceData))
		{
			return {};
		}
	}
	else
	{
		return {};
	}

	std::shared_ptr<FStaticMesh> Mesh = std::make_shared<FStaticMesh>();
	if (!Mesh || !Mesh->Initialize(InDevice, InDeviceContext, std::move(SourceData)))
	{
		return {};
	}

	MeshCache.emplace(MeshCacheKey, Mesh);
	return Mesh;
}

void FStaticMeshManager::Release()
{
	MeshCache.clear();
}
