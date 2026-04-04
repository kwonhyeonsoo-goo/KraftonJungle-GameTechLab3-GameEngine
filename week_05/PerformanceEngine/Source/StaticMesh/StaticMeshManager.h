#pragma once

#include <filesystem>
#include <memory>

#include "Graphics/D3D11/D3D11Common.h"
#include "Types/Map.h"
#include "Types/String.h"

class FStaticMesh;

class FStaticMeshManager
{
public:
	std::shared_ptr<FStaticMesh> LoadStaticMesh(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, const std::wstring& InAssetPath);
	void Release();

	static FString BuildAssetKey(const std::wstring& InAssetPath);

private:
	TMap<FString, std::shared_ptr<FStaticMesh>> MeshCache;
};
