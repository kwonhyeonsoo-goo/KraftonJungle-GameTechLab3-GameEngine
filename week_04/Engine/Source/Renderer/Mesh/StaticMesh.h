#pragma once
#include "Object/Object.h"
#include "Types/CoreTypes.h"

struct FStaticMesh;

class UStaticMesh : public UObject
{
public:
	DECLARE_RTTI(UStaticMesh, UObject)
	void SetStaticMeshAsset(FStaticMesh* InMesh) { MeshAsset = InMesh; }
	FStaticMesh* GetAsset() const { return MeshAsset; }

	/** ProjectRoot/{상대 경로} 반환 **/
	const FString& GetAssetPath() const { return ""; }

private:
	FStaticMesh* MeshAsset = nullptr;

};
