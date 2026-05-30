#include "Physics/PhysicsAssetManager.h"

#include "Animation/Skeleton/SkeletonManager.h"
#include "Asset/AssetPackage.h"
#include "Core/Logging/Log.h"
#include "Object/Object.h"
#include "Physics/PhysicsAsset.h"
#include "Platform/Paths.h"
#include "Serialization/WindowsArchive.h"

#include <algorithm>
#include <filesystem>

namespace
{
    static std::filesystem::path ResolveProjectPath(const FString& Path)
    {
        std::filesystem::path FullPath(FPaths::ToWide(Path));
        if (!FullPath.is_absolute())
        {
            FullPath = std::filesystem::path(FPaths::RootDir()) / FullPath;
        }
        return FullPath.lexically_normal();
    }

    static bool TryGetSourceFileState(const FString& SourcePath, uint64& OutTimestamp, uint64& OutFileSize)
    {
        std::filesystem::path FullPath = ResolveProjectPath(SourcePath);

        if (!std::filesystem::exists(FullPath) || !std::filesystem::is_regular_file(FullPath))
        {
            OutTimestamp = 0;
            OutFileSize  = 0;
            return false;
        }

        OutFileSize          = static_cast<uint64>(std::filesystem::file_size(FullPath));
        const auto WriteTime = std::filesystem::last_write_time(FullPath);
        OutTimestamp         = static_cast<uint64>(WriteTime.time_since_epoch().count());
        return true;
    }

    static FAssetImportMetadata MakeImportMetadata(const FString& SourcePath)
    {
        FAssetImportMetadata Metadata;
        if (SourcePath.empty())
        {
            return Metadata;
        }

        Metadata.SourcePath = FPaths::MakeProjectRelative(SourcePath);
        TryGetSourceFileState(SourcePath, Metadata.SourceTimestamp, Metadata.SourceFileSize);
        return Metadata;
    }

    static FString GetDisplayNameFromPath(const std::filesystem::path& Path)
    {
        return FPaths::ToUtf8(Path.stem().generic_wstring());
    }

}

FPhysicsAssetManager& FPhysicsAssetManager::Get()
{
    static FPhysicsAssetManager Instance;
    return Instance;
}

FString FPhysicsAssetManager::BuildDefaultPhysicsAssetPackagePath(const FString& SkeletonPackagePath)
{
    std::filesystem::path ProjectRelative = std::filesystem::path(FPaths::ToWide(FPaths::MakeProjectRelative(SkeletonPackagePath))).lexically_normal();

    std::filesystem::path AssetPath = (!ProjectRelative.empty() && ProjectRelative.begin()->wstring() == L"Content")
        ? ProjectRelative
        : (std::filesystem::path(L"Content") / ProjectRelative);

    FString Stem = FPaths::ToUtf8(AssetPath.stem().generic_wstring());
    static constexpr const char* SkeletonSuffix = "_Skeleton";
    if (Stem.size() >= std::char_traits<char>::length(SkeletonSuffix) &&
        Stem.compare(Stem.size() - std::char_traits<char>::length(SkeletonSuffix), std::char_traits<char>::length(SkeletonSuffix), SkeletonSuffix) == 0)
    {
        Stem.erase(Stem.size() - std::char_traits<char>::length(SkeletonSuffix));
    }

    AssetPath.replace_filename(FPaths::ToWide(Stem + "_PhysicsAsset.uasset"));
    FPaths::CreateDir(((std::filesystem::path(FPaths::RootDir()) / AssetPath).parent_path()).wstring());
    return FPaths::ToUtf8(AssetPath.generic_wstring());
}

void FPhysicsAssetManager::ScanPhysicsAssets()
{
    AvailablePhysicsAssetFiles.clear();

    namespace fs = std::filesystem;
    const fs::path ContentDir = fs::path(FPaths::RootDir()) / L"Content";
    if (!fs::exists(ContentDir) || !fs::is_directory(ContentDir))
    {
        return;
    }

    for (const auto& Entry : fs::recursive_directory_iterator(ContentDir))
    {
        if (!Entry.is_regular_file())
        {
            continue;
        }

        if (Entry.path().extension() != L".uasset")
        {
            continue;
        }

        const FString RelPath = FPaths::MakeProjectRelative(FPaths::ToUtf8(Entry.path().generic_wstring()));

        EAssetPackageType Type = EAssetPackageType::Unknown;
        if (!FAssetPackage::GetPackageType(RelPath, Type) || Type != EAssetPackageType::PhysicsAsset)
        {
            continue;
        }

        FAssetListItem Item;
        Item.DisplayName = GetDisplayNameFromPath(Entry.path());
        Item.FullPath    = RelPath;
        AvailablePhysicsAssetFiles.push_back(Item);
    }
}

const TArray<FAssetListItem>& FPhysicsAssetManager::GetAvailablePhysicsAssetFiles()
{
    if (AvailablePhysicsAssetFiles.empty())
    {
        ScanPhysicsAssets();
    }

    return AvailablePhysicsAssetFiles;
}

UPhysicsAsset* FPhysicsAssetManager::LoadPhysicsAsset(const FString& PackagePath)
{
    const FString NormalizedPath = FPaths::MakeProjectRelative(PackagePath);

    auto It = PhysicsAssetCaches.find(NormalizedPath);
    if (It != PhysicsAssetCaches.end())
    {
        if (IsValid(It->second))
        {
            return It->second;
        }
        PhysicsAssetCaches.erase(It);
    }

    FWindowsBinReader Reader(NormalizedPath);
    if (!Reader.IsValid())
    {
        UE_LOG("PhysicsAsset load failed: could not open file. Path=%s", NormalizedPath.c_str());
        return nullptr;
    }

    FAssetPackageHeader Header;
    FAssetImportMetadata Metadata;
    if (!FAssetPackage::ReadPackagePrelude(Reader, EAssetPackageType::PhysicsAsset, Header, Metadata))
    {
        UE_LOG("PhysicsAsset load failed: invalid package header. Path=%s", NormalizedPath.c_str());
        return nullptr;
    }

    UPhysicsAsset* PhysicsAsset = UObjectManager::Get().CreateObject<UPhysicsAsset>();
    PhysicsAsset->Serialize(Reader);
    PhysicsAsset->SetAssetPathFileName(NormalizedPath);

    if (!Reader.IsValid())
    {
        UE_LOG("PhysicsAsset load failed: corrupted package. Path=%s", NormalizedPath.c_str());
        UObjectManager::Get().DestroyObject(PhysicsAsset);
        return nullptr;
    }

    PhysicsAssetCaches[NormalizedPath] = PhysicsAsset;
    UE_LOG("PhysicsAsset loaded. Path=%s Bodies=%d Constraints=%d",
        NormalizedPath.c_str(),
        static_cast<int32>(PhysicsAsset->GetBodySetups().size()),
        static_cast<int32>(PhysicsAsset->GetConstraintSetups().size()));
    return PhysicsAsset;
}

bool FPhysicsAssetManager::SavePhysicsAsset(UPhysicsAsset* PhysicsAsset, const FString& PackagePath, const FString& SourcePath)
{
    if (!PhysicsAsset)
    {
        return false;
    }

    const FString NormalizedPath = FPaths::MakeProjectRelative(PackagePath);
    PhysicsAsset->SetAssetPathFileName(NormalizedPath);

    FWindowsBinWriter Writer(NormalizedPath);
    if (!Writer.IsValid())
    {
        UE_LOG("PhysicsAsset save failed: could not open file. Path=%s", NormalizedPath.c_str());
        return false;
    }

    const FAssetImportMetadata Metadata = MakeImportMetadata(SourcePath);
    if (!FAssetPackage::WritePackagePrelude(Writer, EAssetPackageType::PhysicsAsset, Metadata))
    {
        UE_LOG("PhysicsAsset save failed: package prelude write failed. Path=%s", NormalizedPath.c_str());
        return false;
    }

    PhysicsAsset->Serialize(Writer);

    if (!Writer.IsValid())
    {
        UE_LOG("PhysicsAsset save failed: write failed. Path=%s", NormalizedPath.c_str());
        return false;
    }

    PhysicsAssetCaches[NormalizedPath] = PhysicsAsset;
    UE_LOG("PhysicsAsset saved. Path=%s Bodies=%d Constraints=%d",
        NormalizedPath.c_str(),
        static_cast<int32>(PhysicsAsset->GetBodySetups().size()),
        static_cast<int32>(PhysicsAsset->GetConstraintSetups().size()));

    const FString DisplayName = GetDisplayNameFromPath(std::filesystem::path(FPaths::ToWide(NormalizedPath)));
    auto ListIt = std::find_if(
        AvailablePhysicsAssetFiles.begin(),
        AvailablePhysicsAssetFiles.end(),
        [&](const FAssetListItem& Item)
        {
            return Item.FullPath == NormalizedPath;
        }
    );

    if (ListIt == AvailablePhysicsAssetFiles.end())
    {
        FAssetListItem Item;
        Item.DisplayName = DisplayName.empty() ? NormalizedPath : DisplayName;
        Item.FullPath    = NormalizedPath;
        AvailablePhysicsAssetFiles.push_back(Item);
    }
    else
    {
        ListIt->DisplayName = DisplayName.empty() ? NormalizedPath : DisplayName;
    }

    return true;
}

TArray<FAssetListItem> FPhysicsAssetManager::FindPhysicsAssetsForSkeleton(const FSkeletonBinding& Skeleton, bool bAllowSameStructure)
{
    TArray<FAssetListItem> Result;

    for (const FAssetListItem& Item : GetAvailablePhysicsAssetFiles())
    {
        UPhysicsAsset* PhysicsAsset = LoadPhysicsAsset(Item.FullPath);
        if (!PhysicsAsset)
        {
            continue;
        }

        const FSkeletonCompatibilityReport Report = FSkeletonManager::CheckCompatibility(
            PhysicsAsset->GetSkeletonBinding(),
            Skeleton
        );

        if (Report.Result == ESkeletonCompatibilityResult::ExactSkeleton ||
            (bAllowSameStructure && Report.Result == ESkeletonCompatibilityResult::SameStructure))
        {
            Result.push_back(Item);
        }
    }

    return Result;
}
void FPhysicsAssetManager::AddReferencedObjects(FReferenceCollector& Collector)
{
    for (auto& Pair : PhysicsAssetCaches)
    {
        Collector.AddReferencedObject(Pair.second, "FPhysicsAssetManager.PhysicsAssetCaches");
    }
}

void FPhysicsAssetManager::ClearCache()
{
    PhysicsAssetCaches.clear();
    AvailablePhysicsAssetFiles.clear();
}
