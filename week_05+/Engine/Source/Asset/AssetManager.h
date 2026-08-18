#pragma once
#include "CoreMinimal.h"
#include "EngineAPI.h"
#include <string>
#include <filesystem>
#include <d3d11.h>

class UStaticMesh;
struct ID3D11Device;

class ENGINE_API FAssetManager
{
public:
	static FAssetManager& Get();



	// 에셋 이름("sword.obj" 등)을 받아서, 메모리에 로드된 메쉬를 반환 (없으면 로드함)
	UStaticMesh* LoadStaticMesh(ID3D11Device* Device, const FString& AssetName);
	ID3D11ShaderResourceView* LoadTexture(ID3D11Device* Device, const FString& FilePath);
	void InvalidateStaticMesh(const FString& AssetName);
	void ClearCache();
private:
	FAssetManager() = default;
	~FAssetManager();

	FAssetManager(const FAssetManager&) = delete;
	FAssetManager& operator=(const FAssetManager&) = delete;
	// Basic Shape Creator
	UStaticMesh* LoadBasicShape(const FString& ShapeName);
	// 메모리에 올라간 실제 오브젝트들을 들고 있음 (메모리 캐싱)
	TMap<std::string, UStaticMesh*> LoadedMeshes;
	TMap<std::string, ID3D11ShaderResourceView*> LoadedTextures;

};
