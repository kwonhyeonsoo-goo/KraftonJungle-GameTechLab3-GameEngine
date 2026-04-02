#pragma once
#include "CoreMinimal.h"
#include "EngineAPI.h"
#include <string>

// 에셋의 메타데이터를 담는 구조체 (언리얼의 FAssetData 역할)
struct FAssetData
{
	FString AssetName;  // 예: "wood.png"
	FString AssetPath;  // 예: "Assets/Textures/wood.png"
	FString AssetClass; // 예: "Texture", "StaticMesh"
};

class ENGINE_API FAssetRegistry
{
public:
	static FAssetRegistry& Get();

	// 프로젝트 디렉토리를 싹 뒤져서 FAssetData 목록을 만듦
	void SearchAllAssets(const FString& RootDir);

	// 이름으로 데이터 찾기
	bool GetAssetByObjectPath(const FString& ObjectName, FAssetData& OutData) const;

	// 특정 타입(확장자) 에셋 다 가져오기
	TArray<FAssetData> GetAssetsByClass(const FString& AssetClass) const;

private:
	FAssetRegistry() = default;

	// Registry DB: [에셋 이름] -> [에셋 메타데이터]
	TMap<std::string, FAssetData> RegistryData;
};