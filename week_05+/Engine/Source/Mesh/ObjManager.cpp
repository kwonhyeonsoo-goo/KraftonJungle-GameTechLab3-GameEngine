#include "ObjManager.h"
#include "Mesh/StaticMeshRenderData.h"
#include "Object/StaticMesh.h"
#include "Mesh/ObjImporter.h"
#include "Mesh/ObjInfo.h"


TMap<FString, std::shared_ptr<FStaticMeshRenderData>> FObjManager::ObjStaticMeshMap;

namespace
{
	FString BuildObjStaticMeshCacheKey(const FString& PathFileName)
	{
		FString CacheKey = PathFileName;
#if IS_OBJ_VIEWER //버그 가능성. 축 설정 후 다른 캐시로 저장할 수 있습니다.
		CacheKey += "|";
		CacheKey += FObjImporter::BuildImportAxisMappingKey(FObjImporter::GetImportAxisMapping());
#endif
		return CacheKey;
	}
}

FStaticMeshRenderData* FObjManager::LoadObjStaticMeshAsset(const FString& PathFileName)
{
	const FString CacheKey = BuildObjStaticMeshCacheKey(PathFileName);

	auto It = ObjStaticMeshMap.find(CacheKey);
	if (It != ObjStaticMeshMap.end())
		return It->second.get();

	FObjInfo Info;
	if (!FObjImporter::ParseObj(PathFileName, Info))
		return nullptr;

	FStaticMeshRenderData* Cooked = FObjImporter::Cook(Info);
	if (!Cooked) return nullptr;
	Cooked->SetAssetPath(PathFileName);

	ObjStaticMeshMap[CacheKey] = std::shared_ptr<FStaticMeshRenderData>(Cooked);
	return Cooked;
}

void FObjManager::ClearAssetCache(const FString& PathFileName)
{
	for (auto It = ObjStaticMeshMap.begin(); It != ObjStaticMeshMap.end();)
	{
		const FString& CacheKey = It->first;
		const bool bMatchesExactPath = CacheKey == PathFileName;
		const bool bMatchesViewerVariant =
			CacheKey.size() > PathFileName.size() &&
			CacheKey.compare(0, PathFileName.size(), PathFileName) == 0 &&
			CacheKey[PathFileName.size()] == '|';

		if (bMatchesExactPath || bMatchesViewerVariant)
		{
			It = ObjStaticMeshMap.erase(It);
			continue;
		}

		++It;
	}
}

void FObjManager::Clear()
{
	ObjStaticMeshMap.clear();
}
