#include "StaticMesh/ObjParser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "StaticMesh/StaticMesh.h"

namespace
{
	struct FObjFaceIndex
	{
		int32 Position = 0;
		int32 TexCoord = -1;
	};

	struct FVertexKey
	{
		int32 Position = 0;
		int32 TexCoord = -1;

		bool operator==(const FVertexKey& InOther) const
		{
			return Position == InOther.Position && TexCoord == InOther.TexCoord;
		}
	};

	struct FVertexKeyHasher
	{
		size_t operator()(const FVertexKey& InKey) const
		{
			return (static_cast<size_t>(InKey.Position) * 73856093u) ^ (static_cast<size_t>(InKey.TexCoord + 1) * 19349663u);
		}
	};

	using FMaterialPathMap = std::unordered_map<FString, std::filesystem::path>;

	std::string Trim(const std::string& InValue)
	{
		const auto Begin = std::find_if_not(InValue.begin(), InValue.end(), [](unsigned char Ch) { return std::isspace(Ch) != 0; });
		const auto End = std::find_if_not(InValue.rbegin(), InValue.rend(), [](unsigned char Ch) { return std::isspace(Ch) != 0; }).base();
		if (Begin >= End)
		{
			return {};
		}

		return std::string(Begin, End);
	}

	bool StartsWith(const std::string& InValue, const char* InPrefix)
	{
		return InValue.rfind(InPrefix, 0) == 0;
	}

	bool TryParseInt(const std::string& InValue, int32& OutValue)
	{
		if (InValue.empty())
		{
			return false;
		}

		try
		{
			size_t ParsedCharacterCount = 0;
			const int ParsedValue = std::stoi(InValue, &ParsedCharacterCount);
			if (ParsedCharacterCount != InValue.size())
			{
				return false;
			}

			OutValue = ParsedValue;
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	bool ParseFaceIndex(const std::string& InToken, FObjFaceIndex& OutIndex)
	{
		OutIndex = {};
		OutIndex.TexCoord = -1;

		const size_t FirstSlash = InToken.find('/');
		if (FirstSlash == std::string::npos)
		{
			return TryParseInt(InToken, OutIndex.Position);
		}

		const std::string PositionToken = InToken.substr(0, FirstSlash);
		if (!TryParseInt(PositionToken, OutIndex.Position))
		{
			return false;
		}

		const size_t SecondSlash = InToken.find('/', FirstSlash + 1);
		const std::string TexCoordToken =
			SecondSlash == std::string::npos
				? InToken.substr(FirstSlash + 1)
				: InToken.substr(FirstSlash + 1, SecondSlash - FirstSlash - 1);

		if (!TexCoordToken.empty() && !TryParseInt(TexCoordToken, OutIndex.TexCoord))
		{
			return false;
		}

		return true;
	}

	bool ResolveObjIndex(int32 InObjIndex, size_t InElementCount, size_t& OutResolvedIndex)
	{
		if (InObjIndex > 0)
		{
			const size_t ResolvedIndex = static_cast<size_t>(InObjIndex - 1);
			if (ResolvedIndex >= InElementCount)
			{
				return false;
			}

			OutResolvedIndex = ResolvedIndex;
			return true;
		}

		if (InObjIndex < 0)
		{
			const int64 ResolvedIndex = static_cast<int64>(InElementCount) + static_cast<int64>(InObjIndex);
			if (ResolvedIndex < 0 || ResolvedIndex >= static_cast<int64>(InElementCount))
			{
				return false;
			}

			OutResolvedIndex = static_cast<size_t>(ResolvedIndex);
			return true;
		}

		return false;
	}

	void ExpandBounds(FStaticMeshSourceData& InOutSourceData, const FVector& InPosition)
	{
		InOutSourceData.BoundsMin.X = std::min(InOutSourceData.BoundsMin.X, InPosition.X);
		InOutSourceData.BoundsMin.Y = std::min(InOutSourceData.BoundsMin.Y, InPosition.Y);
		InOutSourceData.BoundsMin.Z = std::min(InOutSourceData.BoundsMin.Z, InPosition.Z);

		InOutSourceData.BoundsMax.X = std::max(InOutSourceData.BoundsMax.X, InPosition.X);
		InOutSourceData.BoundsMax.Y = std::max(InOutSourceData.BoundsMax.Y, InPosition.Y);
		InOutSourceData.BoundsMax.Z = std::max(InOutSourceData.BoundsMax.Z, InPosition.Z);
	}

	void ParseMaterialLibrary(const std::filesystem::path& InMaterialPath, FMaterialPathMap& InOutMaterialPathMap)
	{
		if (InMaterialPath.empty())
		{
			return;
		}

		std::ifstream MaterialFile(InMaterialPath);
		if (!MaterialFile)
		{
			return;
		}

		FString CurrentMaterialName;
		std::string Line;
		while (std::getline(MaterialFile, Line))
		{
			const std::string TrimmedLine = Trim(Line);
			if (TrimmedLine.empty() || TrimmedLine[0] == '#')
			{
				continue;
			}

			if (StartsWith(TrimmedLine, "newmtl "))
			{
				CurrentMaterialName = Trim(TrimmedLine.substr(7));
				continue;
			}

			if (!StartsWith(TrimmedLine, "map_Kd ") || CurrentMaterialName.empty())
			{
				continue;
			}

			const std::string RelativeTexturePath = Trim(TrimmedLine.substr(7));
			if (RelativeTexturePath.empty())
			{
				continue;
			}

			InOutMaterialPathMap[CurrentMaterialName] =
				(InMaterialPath.parent_path() / std::filesystem::path(RelativeTexturePath)).lexically_normal();
		}
	}

	int32 FindOrAddMaterial(
		const FString& InMaterialName,
		const FMaterialPathMap& InMaterialPathMap,
		TArray<FStaticMeshSourceData::FMaterial>& InOutMaterials,
		std::unordered_map<FString, int32>& InOutMaterialIndexByName)
	{
		if (InMaterialName.empty())
		{
			return -1;
		}

		const auto ExistingMaterialIt = InOutMaterialIndexByName.find(InMaterialName);
		if (ExistingMaterialIt != InOutMaterialIndexByName.end())
		{
			return ExistingMaterialIt->second;
		}

		FStaticMeshSourceData::FMaterial Material = {};
		Material.Name = InMaterialName;

		const auto MaterialPathIt = InMaterialPathMap.find(InMaterialName);
		if (MaterialPathIt != InMaterialPathMap.end())
		{
			Material.DiffuseTexturePath = MaterialPathIt->second;
		}

		const int32 MaterialIndex = static_cast<int32>(InOutMaterials.size());
		InOutMaterials.push_back(std::move(Material));
		InOutMaterialIndexByName.emplace(InMaterialName, MaterialIndex);
		return MaterialIndex;
	}

	void FinalizeSectionIfNeeded(
		FStaticMeshSourceData& InOutSourceData,
		uint32 InSectionStartIndex,
		int32 InMaterialIndex)
	{
		const uint32 CurrentIndexCount = static_cast<uint32>(InOutSourceData.Indices.size());
		if (CurrentIndexCount <= InSectionStartIndex)
		{
			return;
		}

		InOutSourceData.Sections.push_back({
			InSectionStartIndex,
			CurrentIndexCount - InSectionStartIndex,
			InMaterialIndex
			});
	}
}

bool FObjParser::Parse(const std::filesystem::path& InObjPath, FStaticMeshSourceData& OutSourceData)
{
	OutSourceData = {};

	if (InObjPath.empty())
	{
		return false;
	}

	const std::filesystem::path ObjPath = std::filesystem::absolute(InObjPath).lexically_normal();
	std::ifstream ObjFile(ObjPath);
	if (!ObjFile)
	{
		return false;
	}

	OutSourceData.SourcePath = ObjPath;
	OutSourceData.Vertices.reserve(2048);
	OutSourceData.Indices.reserve(4096);

	TArray<FVector> Positions;
	TArray<FVector2> TexCoords;
	Positions.reserve(2048);
	TexCoords.reserve(2048);

	FMaterialPathMap MaterialPathMap;
	std::unordered_map<FString, int32> MaterialIndexByName;
	std::unordered_map<FVertexKey, uint32, FVertexKeyHasher> VertexIndexMap;

	OutSourceData.BoundsMin = FVector(
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max());
	OutSourceData.BoundsMax = FVector(
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest());

	int32 CurrentMaterialIndex = -1;
	uint32 CurrentSectionStartIndex = 0;
	bool bHasSectionContext = false;

	std::string Line;
	while (std::getline(ObjFile, Line))
	{
		const std::string TrimmedLine = Trim(Line);
		if (TrimmedLine.empty() || TrimmedLine[0] == '#')
		{
			continue;
		}

		if (StartsWith(TrimmedLine, "mtllib "))
		{
			const std::string RelativeMaterialPath = Trim(TrimmedLine.substr(7));
			if (!RelativeMaterialPath.empty())
			{
				ParseMaterialLibrary((ObjPath.parent_path() / std::filesystem::path(RelativeMaterialPath)).lexically_normal(), MaterialPathMap);
			}
			continue;
		}

		if (StartsWith(TrimmedLine, "usemtl "))
		{
			FinalizeSectionIfNeeded(OutSourceData, CurrentSectionStartIndex, CurrentMaterialIndex);
			CurrentMaterialIndex = FindOrAddMaterial(
				Trim(TrimmedLine.substr(7)),
				MaterialPathMap,
				OutSourceData.Materials,
				MaterialIndexByName);
			CurrentSectionStartIndex = static_cast<uint32>(OutSourceData.Indices.size());
			bHasSectionContext = true;
			continue;
		}

		if (StartsWith(TrimmedLine, "v "))
		{
			std::istringstream Stream(TrimmedLine.substr(2));
			float X = 0.0f;
			float Y = 0.0f;
			float Z = 0.0f;
			Stream >> X >> Y >> Z;
			Positions.emplace_back(X, Y, Z);
			continue;
		}

		if (StartsWith(TrimmedLine, "vt "))
		{
			std::istringstream Stream(TrimmedLine.substr(3));
			float U = 0.0f;
			float V = 0.0f;
			Stream >> U >> V;
			TexCoords.emplace_back(U, 1.0f - V);
			continue;
		}

		if (!StartsWith(TrimmedLine, "f "))
		{
			continue;
		}

		if (!bHasSectionContext)
		{
			CurrentSectionStartIndex = static_cast<uint32>(OutSourceData.Indices.size());
			bHasSectionContext = true;
		}

		std::istringstream Stream(TrimmedLine.substr(2));
		std::vector<FObjFaceIndex> FaceIndices;
		std::string Token;
		while (Stream >> Token)
		{
			FObjFaceIndex FaceIndex;
			if (ParseFaceIndex(Token, FaceIndex))
			{
				FaceIndices.push_back(FaceIndex);
			}
		}

		if (FaceIndices.size() < 3)
		{
			continue;
		}

		auto ResolveVertexIndex = [&](const FObjFaceIndex& InFaceIndex, uint32& OutVertexIndex) -> bool
		{
			size_t PositionIndex = 0;
			if (!ResolveObjIndex(InFaceIndex.Position, Positions.size(), PositionIndex))
			{
				return false;
			}

			size_t TexCoordIndex = 0;
			const bool bHasTexCoord =
				InFaceIndex.TexCoord != -1 && ResolveObjIndex(InFaceIndex.TexCoord, TexCoords.size(), TexCoordIndex);

			const FVertexKey VertexKey =
			{
				static_cast<int32>(PositionIndex),
				bHasTexCoord ? static_cast<int32>(TexCoordIndex) : -1
			};

			const auto ExistingVertexIt = VertexIndexMap.find(VertexKey);
			if (ExistingVertexIt != VertexIndexMap.end())
			{
				OutVertexIndex = ExistingVertexIt->second;
				return true;
			}

			const FVector Position = Positions[PositionIndex];
			const FVector2 TexCoord = bHasTexCoord ? TexCoords[TexCoordIndex] : FVector2();

			OutVertexIndex = static_cast<uint32>(OutSourceData.Vertices.size());
			OutSourceData.Vertices.push_back({ Position, TexCoord });
			VertexIndexMap.emplace(VertexKey, OutVertexIndex);
			ExpandBounds(OutSourceData, Position);
			return true;
		};

		for (size_t TriangleIndex = 1; TriangleIndex + 1 < FaceIndices.size(); ++TriangleIndex)
		{
			uint32 TriangleVertexIndices[3] = {};
			if (!ResolveVertexIndex(FaceIndices[0], TriangleVertexIndices[0])
				|| !ResolveVertexIndex(FaceIndices[TriangleIndex], TriangleVertexIndices[1])
				|| !ResolveVertexIndex(FaceIndices[TriangleIndex + 1], TriangleVertexIndices[2]))
			{
				continue;
			}

			OutSourceData.Indices.push_back(TriangleVertexIndices[0]);
			OutSourceData.Indices.push_back(TriangleVertexIndices[1]);
			OutSourceData.Indices.push_back(TriangleVertexIndices[2]);
		}
	}

	FinalizeSectionIfNeeded(OutSourceData, CurrentSectionStartIndex, CurrentMaterialIndex);

	if (!OutSourceData.IsValid())
	{
		OutSourceData = {};
		return false;
	}

	return true;
}
