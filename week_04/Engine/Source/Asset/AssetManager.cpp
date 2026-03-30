#include "AssetManager.h"

TMap<FString, FStaticMesh*> FAssetManager::StaticMeshCache;
TMap<FString, FMaterial*>   FAssetManager::MaterialCache;
TMap<FString, FTexture*>    FAssetManager::TextureCache;
ID3D11Device* FAssetManager::Device = nullptr;
