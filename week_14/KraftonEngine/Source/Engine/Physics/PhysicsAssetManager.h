#pragma once

#include "Object/GarbageCollection.h"

#include "Asset/AssetRegistry.h"
#include "Animation/Skeleton/SkeletonTypes.h"
#include "Core/Types/CoreTypes.h"

class UPhysicsAsset;

class FPhysicsAssetManager : public FGCObject
{
public:
    static FPhysicsAssetManager& Get();

    UPhysicsAsset* LoadPhysicsAsset(const FString& PackagePath);

    bool SavePhysicsAsset(UPhysicsAsset* PhysicsAsset, const FString& PackagePath, const FString& SourcePath = FString());

    const TArray<FAssetListItem>& GetAvailablePhysicsAssetFiles();
    void                          ScanPhysicsAssets();

    TArray<FAssetListItem> FindPhysicsAssetsForSkeleton(const FSkeletonBinding& Skeleton, bool bAllowSameStructure = false);

    static FString BuildDefaultPhysicsAssetPackagePath(const FString& SkeletonPackagePath);

    const char* GetReferencerName() const override { return "FPhysicsAssetManager"; }
    void AddReferencedObjects(FReferenceCollector& Collector) override;

    void ClearCache();

private:
    FPhysicsAssetManager() = default;

private:
    TMap<FString, UPhysicsAsset*> PhysicsAssetCaches;
    TArray<FAssetListItem>        AvailablePhysicsAssetFiles;
};
