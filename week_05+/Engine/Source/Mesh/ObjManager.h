#pragma once
#include "CoreMinimal.h"
#include <memory>

class FStaticMeshRenderData;
class UStaticMesh;

class ENGINE_API FObjManager
{
public:
	static FStaticMeshRenderData* LoadObjStaticMeshAsset(const FString& PathFileName);
	static void ClearAssetCache(const FString& PathFileName);

	static void Clear();

private:
	static TMap<FString, std::shared_ptr<FStaticMeshRenderData>> ObjStaticMeshMap;
};
