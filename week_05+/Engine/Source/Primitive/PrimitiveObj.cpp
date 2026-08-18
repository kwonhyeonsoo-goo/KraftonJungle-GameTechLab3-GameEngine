#include "PrimitiveObj.h"
#include "Core/Paths.h"
#include <cfloat>
#include <fstream>
#include <sstream>

namespace
{
	FString BuildObjCacheKey(const FString& FilePath, FPrimitiveObj::EImportAxisMode ImportAxisMode)
	{
		switch (ImportAxisMode)
		{
		case FPrimitiveObj::EImportAxisMode::YUpToZUp:
			return FilePath + "|YUpToZUp";
		case FPrimitiveObj::EImportAxisMode::ZUp:
		default:
			return FilePath + "|ZUp";
		}
	}

	FVector ApplyImportAxisTransform(const FVector& InVector, FPrimitiveObj::EImportAxisMode ImportAxisMode)
	{
		switch (ImportAxisMode)
		{
		case FPrimitiveObj::EImportAxisMode::YUpToZUp:
			return { InVector.X, -InVector.Z, InVector.Y };
		case FPrimitiveObj::EImportAxisMode::ZUp:
		default:
			return InVector;
		}
	}

	void RecenterMeshToOrigin(FMeshData& MeshData)
	{
		if (MeshData.Vertices.empty())
		{
			return;
		}

		FVector MinCoord(FLT_MAX, FLT_MAX, FLT_MAX);
		FVector MaxCoord(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		for (const FPrimitiveVertex& Vertex : MeshData.Vertices)
		{
			MinCoord.X = (std::min)(MinCoord.X, Vertex.Position.X);
			MinCoord.Y = (std::min)(MinCoord.Y, Vertex.Position.Y);
			MinCoord.Z = (std::min)(MinCoord.Z, Vertex.Position.Z);

			MaxCoord.X = (std::max)(MaxCoord.X, Vertex.Position.X);
			MaxCoord.Y = (std::max)(MaxCoord.Y, Vertex.Position.Y);
			MaxCoord.Z = (std::max)(MaxCoord.Z, Vertex.Position.Z);
		}

		const FVector Center = (MaxCoord - MinCoord) * 0.5f + MinCoord;
		for (FPrimitiveVertex& Vertex : MeshData.Vertices)
		{
			Vertex.Position -= Center;
		}
	}
}

FPrimitiveObj::EImportAxisMode FPrimitiveObj::ImportAxisMode = FPrimitiveObj::EImportAxisMode::ZUp;

FPrimitiveObj::FPrimitiveObj()
{
	MeshData = std::make_shared<FMeshData>();
}

FPrimitiveObj::FPrimitiveObj(const FString& FilePath)
{
	MeshData = std::make_shared<FMeshData>();
	LoadObj(FilePath);
}

void FPrimitiveObj::LoadObj(const FString& FilePath)
{
	SetPrimitiveFileName(FilePath);

	const FString CacheKey = BuildObjCacheKey(FilePath, ImportAxisMode);
	auto Cached = GetCached(CacheKey);
	if (Cached)
	{
		MeshData = Cached;
		return;
	}

	std::ifstream File(FPaths::ToWide(FPaths::ToAbsolutePath(FilePath)));
	if (!File.is_open())
	{
		printf("[OBJ] Failed to open: %s\n", FilePath.c_str());
		return;
	}

	std::vector<FVector>  Positions;
	std::vector<FVector>  Normals;   
	std::vector<FVector2> UVs;

	uint32 CurrentMaterialIndex = 0;  
	uint32 SectionStartIndex = 0;
	bool   bHasSection = false;

	// Section 마무리 람다
	auto FinishSection = [&]()
	{
		uint32 CurCount = static_cast<uint32>(MeshData->Indices.size());
		if (CurCount > SectionStartIndex)
		{
			FMeshSection Sec;
			Sec.FirstIndex = SectionStartIndex;
			Sec.IndexCount = CurCount - SectionStartIndex;
			Sec.MaterialIndex = CurrentMaterialIndex;
			MeshData->Sections.push_back(Sec);
		}
	};

	std::string Line;
	while (std::getline(File, Line))
	{
		std::stringstream SS(Line);
		std::string Type;
		SS >> Type;

		if (Type == "v")
		{
			FVector Pos;
			SS >> Pos.X >> Pos.Y >> Pos.Z;
			Positions.push_back(Pos);
		}
		//  Normal 파싱
		else if (Type == "vn")
		{
			FVector N;
			SS >> N.X >> N.Y >> N.Z;
			Normals.push_back(N);
		}
		else if (Type == "vt")
		{
			FVector2 UV;
			SS >> UV.X >> UV.Y;
			UV.Y = 1.0f - UV.Y;
			UVs.push_back(UV);
		}
		// usemtl → Section 경계
		else if (Type == "usemtl")
		{
			if (bHasSection)
			{
				FinishSection();
				CurrentMaterialIndex++;
			}
			SectionStartIndex = static_cast<uint32>(MeshData->Indices.size());
			bHasSection = true;
		}
		else if (Type == "f")
		{
			if (!bHasSection)
			{
				bHasSection = true;
				SectionStartIndex = 0;
			}

			std::string VStr;

			//  Normal 인덱스도 파싱 (v/vt/vn 형식 지원)
			struct FIndex
			{
				uint32 PosIdx = 0;
				uint32 UVIdx = 0;
				uint32 NrmIdx = 0;
				bool bHasUV = false;
				bool bHasNrm = false;
			};

			std::vector<FIndex> Face;

			while (SS >> VStr)
			{
				std::stringstream VSS(VStr);
				std::string S0, S1, S2;

				std::getline(VSS, S0, '/');

				FIndex Idx{};
				Idx.PosIdx = std::stoi(S0) - 1;

				if (std::getline(VSS, S1, '/'))
				{
					if (!S1.empty()) { Idx.UVIdx = std::stoi(S1) - 1; Idx.bHasUV = true; }
					if (std::getline(VSS, S2, '/'))
					{
						if (!S2.empty()) { Idx.NrmIdx = std::stoi(S2) - 1; Idx.bHasNrm = true; }
					}
				}
				Face.push_back(Idx);
			}

			// Fan triangulation
			for (size_t i = 1; i + 1 < Face.size(); ++i)
			{
				FIndex Tri[3] = { Face[0], Face[i], Face[i + 1] };

			
				FVector FaceNormal(0, 0, 0);
				if (!Tri[0].bHasNrm)
				{
					FVector E1 = Positions[Tri[1].PosIdx] - Positions[Tri[0].PosIdx];
					FVector E2 = Positions[Tri[2].PosIdx] - Positions[Tri[0].PosIdx];
					FaceNormal = FVector::CrossProduct(E1, E2);
					float Len = FaceNormal.Size();
					if (Len > 0.0001f) FaceNormal = FaceNormal / Len;
				}

				uint32 BaseIndex = static_cast<uint32>(MeshData->Vertices.size());

				for (int j = 0; j < 3; ++j)
				{
					FPrimitiveVertex V{};
					V.Position = ApplyImportAxisTransform(Positions[Tri[j].PosIdx], ImportAxisMode);
					V.Normal = ApplyImportAxisTransform(Tri[j].bHasNrm ? Normals[Tri[j].NrmIdx] : FaceNormal, ImportAxisMode);  
					V.UV = Tri[j].bHasUV ? UVs[Tri[j].UVIdx] : FVector2(0, 0);
					V.Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f); 

					MeshData->Vertices.push_back(V);
				}

				MeshData->Indices.push_back(BaseIndex + 0);
				MeshData->Indices.push_back(BaseIndex + 1);
				MeshData->Indices.push_back(BaseIndex + 2);
			}
		}
	}


	FinishSection();


	if (MeshData->Sections.empty() && !MeshData->Indices.empty())
	{
		FMeshSection Sec;
		Sec.FirstIndex = 0;
		Sec.IndexCount = static_cast<uint32>(MeshData->Indices.size());
		Sec.MaterialIndex = 0;
		MeshData->Sections.push_back(Sec);
	}

	RecenterMeshToOrigin(*MeshData);
	MeshData->Topology = EMeshTopology::EMT_TriangleList;
	MeshData->UpdateLocalBound();

	RegisterMeshData(CacheKey, MeshData);
}

void FPrimitiveObj::SetImportAxisMode(EImportAxisMode InMode)
{
	ImportAxisMode = InMode;
}

FPrimitiveObj::EImportAxisMode FPrimitiveObj::GetImportAxisMode()
{
	return ImportAxisMode;
}
