#include "Mesh/Importer/Fbx/FbxMaterialImporter.h"
#include "Materials/MaterialManager.h"
#include "Materials/Material.h"
#include "Platform/Paths.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <system_error>

namespace
{
	namespace fs = std::filesystem;

	FString NormalizeTexturePathSeparators(FString Path)
	{
		std::replace(Path.begin(), Path.end(), '\\', '/');
		return Path;
	}

	fs::path ToFilesystemPath(const FString& Path)
	{
		fs::path Result(FPaths::ToWide(NormalizeTexturePathSeparators(Path)));
		if (!Result.empty() && !Result.is_absolute())
		{
			Result = fs::path(FPaths::RootDir()) / Result;
		}
		return Result;
	}

	std::wstring ToLower(std::wstring Value)
	{
		std::transform(Value.begin(), Value.end(), Value.begin(), [](wchar_t Ch)
		{
			return static_cast<wchar_t>(std::towlower(Ch));
		});
		return Value;
	}

	FString ToLower(FString Value)
	{
		std::transform(Value.begin(), Value.end(), Value.begin(), [](unsigned char Ch)
		{
			return static_cast<char>(std::tolower(Ch));
		});
		return Value;
	}

	bool IsSupportedTextureExtension(const fs::path& Path)
	{
		const std::wstring Ext = ToLower(Path.extension().wstring());
		return Ext == L".png" || Ext == L".jpg" || Ext == L".jpeg" || Ext == L".tga" || Ext == L".bmp" || Ext == L".dds";
	}

	const TArray<std::wstring>& GetTextureExtensionFallbacks()
	{
		static const TArray<std::wstring> Extensions = {
			L".png", L".jpg", L".jpeg", L".tga", L".bmp", L".dds"
		};
		return Extensions;
	}

	bool TryFindCaseInsensitive(const fs::path& Candidate, fs::path& OutPath)
	{
		std::error_code Ec;
		if (fs::exists(Candidate, Ec) && fs::is_regular_file(Candidate, Ec))
		{
			OutPath = Candidate;
			return true;
		}

		const fs::path Parent = Candidate.parent_path();
		if (Parent.empty() || !fs::exists(Parent, Ec) || !fs::is_directory(Parent, Ec))
		{
			return false;
		}

		const std::wstring Wanted = ToLower(Candidate.filename().wstring());
		for (const fs::directory_entry& Entry : fs::directory_iterator(Parent, Ec))
		{
			if (Ec || !Entry.is_regular_file())
			{
				continue;
			}

			if (ToLower(Entry.path().filename().wstring()) == Wanted)
			{
				OutPath = Entry.path();
				return true;
			}
		}

		return false;
	}

	bool TryResolveTextureCandidate(const fs::path& Candidate, fs::path& OutPath)
	{
		if (Candidate.empty())
		{
			return false;
		}

		if (TryFindCaseInsensitive(Candidate, OutPath))
		{
			return true;
		}

		const fs::path Parent = Candidate.parent_path();
		const fs::path Stem = Candidate.stem();
		if (Parent.empty() || Stem.empty())
		{
			return false;
		}

		for (const std::wstring& Extension : GetTextureExtensionFallbacks())
		{
			fs::path Alternative = Parent / (Stem.wstring() + Extension);
			if (TryFindCaseInsensitive(Alternative, OutPath))
			{
				return true;
			}
		}

		return false;
	}

	void AddUniquePath(TArray<fs::path>& Paths, const fs::path& Path)
	{
		if (Path.empty())
		{
			return;
		}

		for (const fs::path& Existing : Paths)
		{
			if (Existing == Path)
			{
				return;
			}
		}
		Paths.push_back(Path);
	}

	TArray<fs::path> BuildTextureSearchDirectories(const FString& FbxSourcePath)
	{
		TArray<fs::path> Directories;
		const fs::path FbxPath = ToFilesystemPath(FbxSourcePath);
		const fs::path FbxDir = FbxPath.parent_path();
		const fs::path ParentDir = FbxDir.parent_path();

		AddUniquePath(Directories, FbxDir);
		AddUniquePath(Directories, FbxDir / L"textures");
		AddUniquePath(Directories, FbxDir / L"Textures");
		AddUniquePath(Directories, ParentDir);
		AddUniquePath(Directories, ParentDir / L"textures");
		AddUniquePath(Directories, ParentDir / L"Textures");
		return Directories;
	}

	TArray<fs::path> BuildTextureCandidates(const FString& RawTexturePath, const FString& FbxSourcePath)
	{
		TArray<fs::path> Candidates;
		const FString NormalizedRaw = NormalizeTexturePathSeparators(RawTexturePath);
		const fs::path RawPath(FPaths::ToWide(NormalizedRaw));
		if (RawPath.empty())
		{
			return Candidates;
		}

		const fs::path FileName = RawPath.filename();
		const fs::path FbxPath = ToFilesystemPath(FbxSourcePath);
		const fs::path FbxDir = FbxPath.parent_path();
		const fs::path ParentDir = FbxDir.parent_path();

		AddUniquePath(Candidates, RawPath);
		AddUniquePath(Candidates, FbxDir / RawPath);
		AddUniquePath(Candidates, FbxDir / FileName);
		AddUniquePath(Candidates, FbxDir / L"textures" / FileName);
		AddUniquePath(Candidates, FbxDir / L"Textures" / FileName);
		AddUniquePath(Candidates, ParentDir / RawPath);
		AddUniquePath(Candidates, ParentDir / FileName);
		AddUniquePath(Candidates, ParentDir / L"textures" / FileName);
		AddUniquePath(Candidates, ParentDir / L"Textures" / FileName);
		return Candidates;
	}

	FString CopyResolvedTextureToProject(const fs::path& FoundPath, const FString& FbxSourcePath)
	{
		if (FoundPath.empty())
		{
			return FString();
		}

		const fs::path FbxPath = ToFilesystemPath(FbxSourcePath);
		const std::wstring SubFolder = FbxPath.stem().wstring();
		const fs::path DestRelDir = fs::path(L"Content") / L"Texture" / L"Auto" / SubFolder;
		const fs::path DestAbsDir = fs::path(FPaths::RootDir()) / DestRelDir;

		std::error_code Ec;
		fs::create_directories(DestAbsDir, Ec);

		const fs::path DestAbsPath = DestAbsDir / FoundPath.filename();
		fs::copy_file(FoundPath, DestAbsPath, fs::copy_options::overwrite_existing, Ec);
		if (Ec)
		{
			return FPaths::MakeProjectRelative(FPaths::ToUtf8(FoundPath.generic_wstring()));
		}

		const fs::path DestRelPath = DestRelDir / FoundPath.filename();
		return FPaths::ToUtf8(DestRelPath.generic_wstring());
	}

	FString ImportTextureToProject(const FString& RawTexturePath, const FString& FbxSourcePath)
	{
		if (RawTexturePath.empty())
		{
			return FString();
		}

		fs::path FoundPath;
		for (const fs::path& Candidate : BuildTextureCandidates(RawTexturePath, FbxSourcePath))
		{
			if (TryResolveTextureCandidate(Candidate, FoundPath))
			{
				return CopyResolvedTextureToProject(FoundPath, FbxSourcePath);
			}
		}

		// 실제 파일을 못 찾으면 기존 동작 유지 (경로만 정리)
		return FPaths::MakeProjectRelative(NormalizeTexturePathSeparators(RawTexturePath));
	}

	bool ContainsAnyToken(const FString& Text, std::initializer_list<const char*> Tokens)
	{
		for (const char* Token : Tokens)
		{
			if (Text.find(Token) != FString::npos)
			{
				return true;
			}
		}
		return false;
	}

	bool EndsWithToken(const FString& Text, const char* Token)
	{
		const size_t TokenLength = std::strlen(Token);
		if (Text.size() < TokenLength)
		{
			return false;
		}

		return Text.compare(Text.size() - TokenLength, TokenLength, Token) == 0;
	}

	FString GetLowerTextureStem(const FString& TexturePath)
	{
		const fs::path Path(FPaths::ToWide(NormalizeTexturePathSeparators(TexturePath)));
		return ToLower(FPaths::ToUtf8(Path.stem().wstring()));
	}

	bool HasNormalTextureToken(const FString& Stem)
	{
		return EndsWithToken(Stem, "_n") ||
			EndsWithToken(Stem, "-n") ||
			ContainsAnyToken(Stem, { "_normal", "-normal", " normal", "normal", "_norm", "-norm", " norm", "_bump", "-bump", " bump", "bump" });
	}

	bool HasColorTextureToken(const FString& Stem)
	{
		return EndsWithToken(Stem, "_c") ||
			EndsWithToken(Stem, "-c") ||
			ContainsAnyToken(Stem, { "_diff", "-diff", " diff", "diffuse", "albedo", "basecolor", "_color", "-color", " color", "color", "_col", "-col", " col" });
	}

	bool HasPackedDataTextureToken(const FString& Stem)
	{
		return EndsWithToken(Stem, "_ro") ||
			EndsWithToken(Stem, "-ro") ||
			ContainsAnyToken(Stem, { "rough", "_spec", "-spec", " spec", "metal", "_ao", "-ao", " ao" });
	}

	bool IsLikelyNormalTexturePath(const FString& TexturePath)
	{
		const FString Stem = GetLowerTextureStem(TexturePath);
		return HasNormalTextureToken(Stem);
	}

	bool IsLikelyColorTexturePath(const FString& TexturePath)
	{
		const FString Stem = GetLowerTextureStem(TexturePath);
		return HasColorTextureToken(Stem);
	}

	int32 ScoreTextureForRole(const fs::path& TexturePath, const FString& MaterialName, bool bNormalMap)
	{
		const FString Stem = ToLower(FPaths::ToUtf8(TexturePath.stem().wstring()));
		const FString File = ToLower(FPaths::ToUtf8(TexturePath.filename().wstring()));
		const FString Mat  = ToLower(MaterialName);
		int32 Score = 0;

		if (Mat.find(Stem) != FString::npos || Stem.find(Mat) != FString::npos)
		{
			Score += 80;
		}

		if (bNormalMap)
		{
			if (HasNormalTextureToken(Stem)) Score += 120;
			if (HasColorTextureToken(Stem) || HasPackedDataTextureToken(Stem)) Score -= 100;
		}
		else
		{
			if (HasColorTextureToken(Stem)) Score += 120;
			if (HasNormalTextureToken(Stem) || HasPackedDataTextureToken(Stem)) Score -= 100;
		}

		if (File.find("car") != FString::npos && Mat.find("car") != FString::npos)
		{
			Score += 10;
		}
		return Score;
	}

	FString FindBestTextureByRole(const FString& FbxSourcePath, const FString& MaterialName, bool bNormalMap)
	{
		fs::path BestPath;
		int32 BestScore = bNormalMap ? 30 : 30;
		for (const fs::path& Directory : BuildTextureSearchDirectories(FbxSourcePath))
		{
			std::error_code Ec;
			if (Directory.empty() || !fs::exists(Directory, Ec) || !fs::is_directory(Directory, Ec))
			{
				continue;
			}

			for (const fs::directory_entry& Entry : fs::directory_iterator(Directory, Ec))
			{
				if (Ec || !Entry.is_regular_file() || !IsSupportedTextureExtension(Entry.path()))
				{
					continue;
				}

				const int32 Score = ScoreTextureForRole(Entry.path(), MaterialName, bNormalMap);
				if (Score > BestScore)
				{
					BestScore = Score;
					BestPath = Entry.path();
				}
			}
		}

		return BestPath.empty() ? FString() : CopyResolvedTextureToProject(BestPath, FbxSourcePath);
	}

	FString ReadFirstTextureFromProperty(const FbxProperty& Property, const FString& FbxSourcePath)
	{
		if (!Property.IsValid())
		{
			return FString();
		}

		const int32 TextureCount = Property.GetSrcObjectCount<FbxTexture>();
		for (int32 TextureIndex = 0; TextureIndex < TextureCount; ++TextureIndex)
		{
			FbxFileTexture* Texture = Property.GetSrcObject<FbxFileTexture>(TextureIndex);
			if (Texture)
			{
				FString Imported = ImportTextureToProject(Texture->GetFileName(), FbxSourcePath);
				if (!Imported.empty())
				{
					return Imported;
				}
			}
		}

		return FString();
	}
}

void FFbxMaterialImporter::CollectMaterials(FbxScene* Scene, FFbxImportContext& Context)
{
	Context.Materials.clear();
	Context.MaterialToSlotIndex.clear();

	if (!Scene)
	{
		return;
	}

	const int32 MaterialCount = Scene->GetMaterialCount();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		FbxSurfaceMaterial* Material = Scene->GetMaterial(MaterialIndex);
		if (!Material)
		{
			continue;
		}

		FFbxImportedMaterialInfo MaterialInfo;
		MaterialInfo.Name = Material->GetName();
		MaterialInfo.DiffuseColor = FVector(1.0f, 1.0f, 1.0f);

		FbxProperty DiffuseProp = Material->FindProperty(FbxSurfaceMaterial::sDiffuse);
		if (DiffuseProp.IsValid())
		{
			FbxDouble3 Color = DiffuseProp.Get<FbxDouble3>();
			MaterialInfo.DiffuseColor = FVector(static_cast<float>(Color[0]), static_cast<float>(Color[1]), static_cast<float>(Color[2]));
			const FString DiffuseTexturePath = ReadFirstTextureFromProperty(DiffuseProp, Context.SourcePath);
			if (!DiffuseTexturePath.empty())
			{
				if (IsLikelyNormalTexturePath(DiffuseTexturePath))
				{
					MaterialInfo.NormalTexturePath = DiffuseTexturePath;
				}
				else
				{
					MaterialInfo.DiffuseTexturePath = DiffuseTexturePath;
				}
			}
		}

		FbxProperty NormalProp = Material->FindProperty(FbxSurfaceMaterial::sNormalMap);
		const FString NormalTexturePath = ReadFirstTextureFromProperty(NormalProp, Context.SourcePath);
		if (!NormalTexturePath.empty())
		{
			if (IsLikelyColorTexturePath(NormalTexturePath) && MaterialInfo.DiffuseTexturePath.empty())
			{
				MaterialInfo.DiffuseTexturePath = NormalTexturePath;
			}
			else
			{
				MaterialInfo.NormalTexturePath = NormalTexturePath;
			}
		}

		if (MaterialInfo.NormalTexturePath.empty())
		{
			FbxProperty BumpProp = Material->FindProperty(FbxSurfaceMaterial::sBump);
			const FString BumpTexturePath = ReadFirstTextureFromProperty(BumpProp, Context.SourcePath);
			if (!BumpTexturePath.empty())
			{
				if (IsLikelyColorTexturePath(BumpTexturePath) && MaterialInfo.DiffuseTexturePath.empty())
				{
					MaterialInfo.DiffuseTexturePath = BumpTexturePath;
				}
				else
				{
					MaterialInfo.NormalTexturePath = BumpTexturePath;
				}
			}
		}

		if (MaterialInfo.DiffuseTexturePath.empty())
		{
			MaterialInfo.DiffuseTexturePath = FindBestTextureByRole(Context.SourcePath, MaterialInfo.Name, false);
		}
		if (MaterialInfo.NormalTexturePath.empty())
		{
			MaterialInfo.NormalTexturePath = FindBestTextureByRole(Context.SourcePath, MaterialInfo.Name, true);
		}

		const int32 GlobalIndex = static_cast<int32>(Context.Materials.size());
		Context.Materials.push_back(MaterialInfo);
		Context.MaterialToSlotIndex[Material] = GlobalIndex;
	}
}

int32 FFbxMaterialImporter::GetMaterialIndex(FbxMesh* Mesh, int32 PolygonIndex)
{
	FbxLayerElementMaterial* LayerElementMaterial = Mesh ? Mesh->GetElementMaterial() : nullptr;
	if (!LayerElementMaterial)
	{
		return -1;
	}

	FbxLayerElementArrayTemplate<int32>& MaterialIndices = LayerElementMaterial->GetIndexArray();
	switch (LayerElementMaterial->GetMappingMode())
	{
	case FbxLayerElement::eAllSame:
		return MaterialIndices[0];
	case FbxLayerElement::eByPolygon:
		return MaterialIndices[PolygonIndex];
	default:
		return 0;
	}
}

void FFbxMaterialImporter::BuildStaticMaterials(const FFbxImportContext& Context, TArray<FStaticMaterial>& OutMaterials)
{
	OutMaterials.clear();
	OutMaterials.reserve(Context.Materials.size());

	for (const FFbxImportedMaterialInfo& MaterialInfo : Context.Materials)
	{
		FStaticMaterial NewMaterial;
		NewMaterial.MaterialSlotName = MaterialInfo.Name;
		NewMaterial.MaterialInterface = FMaterialManager::Get().GetOrCreateMaterial(CreateOrUpdateMaterialAsset(MaterialInfo));
		OutMaterials.push_back(NewMaterial);
	}
}

void FFbxMaterialImporter::BuildSkeletalMaterials(const FFbxImportContext& Context, const TArray<FSkeletalMeshSection>& Sections, TArray<FSkeletalMaterial>& OutMaterials, TArray<FSkeletalMeshSection>& InOutSections)
{
	OutMaterials.clear();
	OutMaterials.reserve(Context.Materials.size());

	for (const FFbxImportedMaterialInfo& MaterialInfo : Context.Materials)
	{
		const FString MaterialPath = CreateOrUpdateMaterialAsset(MaterialInfo);
		UMaterial* MaterialObject = FMaterialManager::Get().GetOrCreateMaterial(MaterialPath);

		FSkeletalMaterial NewMaterial;
		NewMaterial.MaterialInterface = MaterialObject;
		NewMaterial.MaterialSlotName = MaterialInfo.Name;
		NewMaterial.MaterialPath = MaterialPath;
		OutMaterials.push_back(NewMaterial);
	}

	bool bNeedsNoneSlot = OutMaterials.empty();
	for (const FSkeletalMeshSection& Section : Sections)
	{
		if (Section.MaterialSlotName == "None")
		{
			bNeedsNoneSlot = true;
			break;
		}
	}

	if (bNeedsNoneSlot)
	{
		FSkeletalMaterial DefaultMaterial;
		DefaultMaterial.MaterialInterface = FMaterialManager::Get().GetOrCreateMaterial("None");
		DefaultMaterial.MaterialSlotName = "None";
		DefaultMaterial.MaterialPath = DefaultMaterial.MaterialInterface
			? DefaultMaterial.MaterialInterface->GetAssetPathFileName()
			: FString();
		OutMaterials.push_back(DefaultMaterial);

		const int32 NoneMaterialIndex = static_cast<int32>(OutMaterials.size()) - 1;
		for (FSkeletalMeshSection& Section : InOutSections)
		{
			if (Section.MaterialSlotName == "None")
			{
				Section.MaterialIndex = NoneMaterialIndex;
			}
		}
	}
}

FString FFbxMaterialImporter::CreateOrUpdateMaterialAsset(const FFbxImportedMaterialInfo& MaterialInfo)
{
	const FString UassetPath = "Content/Material/Auto/" + MaterialInfo.Name + ".uasset";

	std::filesystem::create_directories(FPaths::ToWide("Content/Material/Auto"));

	const FVector4 SectionColor = MaterialInfo.DiffuseTexturePath.empty()
		? FVector4(MaterialInfo.DiffuseColor.X, MaterialInfo.DiffuseColor.Y, MaterialInfo.DiffuseColor.Z, 1.0f)
		: FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	const FString DiffuseTex = MaterialInfo.DiffuseTexturePath.empty() ? FString() : FPaths::MakeProjectRelative(MaterialInfo.DiffuseTexturePath);
	const FString NormalTex  = MaterialInfo.NormalTexturePath.empty()  ? FString() : FPaths::MakeProjectRelative(MaterialInfo.NormalTexturePath);

	// JSON 없이 머티리얼을 직접 빌드해 .uasset(바이너리)으로 저장.
	FMaterialManager::Get().CreateImportedMaterialAsset(UassetPath, SectionColor, DiffuseTex, NormalTex);
	return UassetPath;
}
