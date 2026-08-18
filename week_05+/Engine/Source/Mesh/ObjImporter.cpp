
#include "ObjImporter.h"
#include "ObjInfo.h"
#include "Core/Paths.h"
#include "StaticMeshRenderData.h"
#include "Types/Map.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <sstream>

#include <cfloat>
#include "Asset/AssetRegistry.h"
#include "Asset/AssetManager.h"

namespace
{
	constexpr float ImportScaleTolerance = 1.0e-4f;

	int32 GetSourceAxisIndex(FObjImporter::EAxisDirection AxisDirection)
	{
		switch (AxisDirection)
		{
		case FObjImporter::EAxisDirection::PositiveX:
		case FObjImporter::EAxisDirection::NegativeX:
			return 0;
		case FObjImporter::EAxisDirection::PositiveY:
		case FObjImporter::EAxisDirection::NegativeY:
			return 1;
		case FObjImporter::EAxisDirection::PositiveZ:
		case FObjImporter::EAxisDirection::NegativeZ:
		default:
			return 2;
		}
	}

	float GetAxisSign(FObjImporter::EAxisDirection AxisDirection)
	{
		switch (AxisDirection)
		{
		case FObjImporter::EAxisDirection::NegativeX:
		case FObjImporter::EAxisDirection::NegativeY:
		case FObjImporter::EAxisDirection::NegativeZ:
			return -1.0f;
		case FObjImporter::EAxisDirection::PositiveX:
		case FObjImporter::EAxisDirection::PositiveY:
		case FObjImporter::EAxisDirection::PositiveZ:
		default:
			return 1.0f;
		}
	}

	float GetAxisComponent(const FVector& InVector, int32 AxisIndex)
	{
		switch (AxisIndex)
		{
		case 0:
			return InVector.X;
		case 1:
			return InVector.Y;
		case 2:
		default:
			return InVector.Z;
		}
	}

	FVector ApplyImportAxisMapping(const FVector& InVector, const FObjImporter::FImportAxisMapping& ImportAxisMapping)
	{
		auto MapAxis = [&](FObjImporter::EAxisDirection AxisDirection)
		{
			return GetAxisComponent(InVector, GetSourceAxisIndex(AxisDirection)) * GetAxisSign(AxisDirection);
		};

		return FVector(
			MapAxis(ImportAxisMapping.EngineX),
			MapAxis(ImportAxisMapping.EngineY),
			MapAxis(ImportAxisMapping.EngineZ));
	}

	const char* GetAxisDirectionKey(FObjImporter::EAxisDirection AxisDirection)
	{
		switch (AxisDirection)
		{
		case FObjImporter::EAxisDirection::PositiveX:
			return "PX";
		case FObjImporter::EAxisDirection::NegativeX:
			return "NX";
		case FObjImporter::EAxisDirection::PositiveY:
			return "PY";
		case FObjImporter::EAxisDirection::NegativeY:
			return "NY";
		case FObjImporter::EAxisDirection::PositiveZ:
			return "PZ";
		case FObjImporter::EAxisDirection::NegativeZ:
		default:
			return "NZ";
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

FObjImporter::FImportAxisMapping FObjImporter::ImportAxisMapping = FObjImporter::MakeDefaultImportAxisMapping();

FObjImporter::FImportAxisMapping FObjImporter::MakeDefaultImportAxisMapping()
{
	FImportAxisMapping DefaultMapping;
	return DefaultMapping;
}

void FObjImporter::SetImportAxisMapping(const FImportAxisMapping& InMapping)
{
	if (!IsValidImportAxisMapping(InMapping))
	{
		return;
	}

	ImportAxisMapping = InMapping;
}

FObjImporter::FImportAxisMapping FObjImporter::GetImportAxisMapping()
{
	return ImportAxisMapping;
}

bool FObjImporter::IsValidImportAxisMapping(const FImportAxisMapping& InMapping)
{
	const int32 XAxis = GetSourceAxisIndex(InMapping.EngineX);
	const int32 YAxis = GetSourceAxisIndex(InMapping.EngineY);
	const int32 ZAxis = GetSourceAxisIndex(InMapping.EngineZ);
	return XAxis != YAxis && XAxis != ZAxis && YAxis != ZAxis && InMapping.ImportScale > ImportScaleTolerance;
}

FString FObjImporter::BuildImportAxisMappingKey(const FImportAxisMapping& InMapping)
{
	return FString(GetAxisDirectionKey(InMapping.EngineX))
		+ "_"
		+ GetAxisDirectionKey(InMapping.EngineY)
		+ "_"
		+ GetAxisDirectionKey(InMapping.EngineZ)
		+ "_S"
		+ std::to_string(InMapping.ImportScale);
}

// ParseObj — 기존 PrimitiveObj::LoadObj에서 변환
bool FObjImporter::ParseObj(const FString& FilePath, FObjInfo& OutInfo)
{
	std::ifstream File(FPaths::ToWide(FPaths::ToAbsolutePath(FilePath)));

	if (!File.is_open()) return false;
	OutInfo.ObjDirectory = FPaths::ToAbsolutePath(FilePath);
	size_t LastSlash = OutInfo.ObjDirectory.rfind('/');
	if (LastSlash != std::string::npos)
		OutInfo.ObjDirectory = OutInfo.ObjDirectory.substr(0, LastSlash);

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
			OutInfo.Positions.push_back(Pos);
		}
		else if (Type == "vt")
		{
			FVector2 UV;
			SS >> UV.X >> UV.Y;
			UV.Y = 1.0f - UV.Y;
			OutInfo.UVs.push_back(UV);
		}
		else if (Type == "vn")
		{
			FVector N;
			SS >> N.X >> N.Y >> N.Z;
			OutInfo.Normals.push_back(N);
		}
		else if (Type == "usemtl")
		{
			FString MatName;
			SS >> MatName;
			OutInfo.MaterialNames.push_back(MatName);
			OutInfo.SectionStartIndices.push_back(
				(uint32)OutInfo.PositionIndices.size());
		}
		else if (Type == "mtllib")
		{
			std::string MtlFileName;
			// 띄어쓰기가 있는 파일명을 위해 안전하게 파싱
			std::getline(SS >> std::ws, MtlFileName);
			FAssetData MtlData;
			FString MtlPath;
			// AssetManager에게 전체 프로젝트에서 .mtl을 찾아달라고 요청
			if (FAssetRegistry::Get().GetAssetByObjectPath(MtlFileName, MtlData))
			{
				// 찾았다면 AssetData 안의 상대경로를 절대경로로 변환
				MtlPath = FPaths::ToAbsolutePath(MtlData.AssetPath);
			}
			else
			{
				// 못 찾았다면 기존 폴백 로직
				FString AbsPath = FPaths::ToAbsolutePath(FilePath);
				size_t Slash = AbsPath.rfind('/');
				FString ObjDir = (Slash != std::string::npos) ? AbsPath.substr(0, Slash) : AbsPath;
				MtlPath = ObjDir + "/" + MtlFileName;
			}
			ParseMtl(MtlPath, OutInfo.Materials);
		}

		else if (Type == "f")
		{
		
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

				for (int j = 0; j < 3; ++j)
				{
					OutInfo.PositionIndices.push_back(Tri[j].PosIdx);
					OutInfo.UVIndices.push_back(Tri[j].bHasUV ? Tri[j].UVIdx : UINT32_MAX);
					OutInfo.NormalIndices.push_back(Tri[j].bHasNrm ? Tri[j].NrmIdx : UINT32_MAX);
				}
			}
		}
	}
	return true;
}
// FObjImporter::ParseMtl
bool FObjImporter::ParseMtl(const FString& FilePath, TArray<FObjMaterialInfo>& OutMaterials)
{
	std::ifstream File(FPaths::ToWide(FPaths::ToAbsolutePath(FilePath)));
	if (!File.is_open()) return false;

	FObjMaterialInfo* Current = nullptr;

	std::string Line;
	while (std::getline(File, Line))
	{
		std::stringstream SS(Line);
		std::string Type;
		SS >> Type;

		if (Type == "newmtl")
		{
			OutMaterials.push_back({});
			Current = &OutMaterials.back();
			SS >> Current->Name;
		}
		else if (!Current) continue;
		else if (Type == "Kd")  // Diffuse Color
		{
			SS >> Current->DiffuseColor.X
				>> Current->DiffuseColor.Y
				>> Current->DiffuseColor.Z;
		}
		else if (Type == "map_Kd")  // Diffuse Texture
		{
			std::getline(SS >> std::ws, Current->DiffuseTexturePath);
		}
	}
	return true;
}
struct FPrimitiveVertexHash
{
	size_t operator()(const FPrimitiveVertex& V) const
	{
		size_t Hash = 17;
		auto Combine = [&Hash](float Val) {
			Hash = Hash * 31 + std::hash<float>()(Val);
		};
		Combine(V.Position.X); Combine(V.Position.Y); Combine(V.Position.Z);
		Combine(V.Normal.X);   Combine(V.Normal.Y);   Combine(V.Normal.Z);
		Combine(V.UV.X);       Combine(V.UV.Y);
		return Hash;
	}
};

struct FPrimitiveVertexEqual
{
	bool operator()(const FPrimitiveVertex& A, const FPrimitiveVertex& B) const
	{
		const float Tolerance = 1e-5f;
		return std::abs(A.Position.X - B.Position.X) < Tolerance &&
			std::abs(A.Position.Y - B.Position.Y) < Tolerance &&
			std::abs(A.Position.Z - B.Position.Z) < Tolerance &&
			std::abs(A.Normal.X - B.Normal.X) < Tolerance &&
			std::abs(A.Normal.Y - B.Normal.Y) < Tolerance &&
			std::abs(A.Normal.Z - B.Normal.Z) < Tolerance &&
			std::abs(A.UV.X - B.UV.X) < Tolerance &&
			std::abs(A.UV.Y - B.UV.Y) < Tolerance;
	}
};
FStaticMeshRenderData* FObjImporter::Cook(const FObjInfo& Info)
{
	auto MeshData = std::make_shared<FMeshData>();
	const FImportAxisMapping ActiveImportAxisMapping = GetImportAxisMapping();
	TMap<FPrimitiveVertex, uint32, FPrimitiveVertexHash, FPrimitiveVertexEqual> UniqueVertices;
	uint32 TriCount = (uint32)Info.PositionIndices.size();
	for (uint32 i = 0; i < TriCount; ++i)
	{
		FPrimitiveVertex V{};
		V.Position = Info.Positions[Info.PositionIndices[i]];

		// Normal: 없으면 면 노말 계산
		if (Info.NormalIndices[i] != UINT32_MAX)
		{
			V.Normal = Info.Normals[Info.NormalIndices[i]];
		}
		else
		{
			// 삼각형 단위 (i는 3의 배수 기준)
			uint32 Base = (i / 3) * 3;
			FVector P0 = Info.Positions[Info.PositionIndices[Base]];
			FVector P1 = Info.Positions[Info.PositionIndices[Base + 1]];
			FVector P2 = Info.Positions[Info.PositionIndices[Base + 2]];
			FVector E1 = P1 - P0;
			FVector E2 = P2 - P0;
			FVector N = FVector::CrossProduct(E1, E2);
			float Len = N.Size();
			V.Normal = (Len > 0.0001f) ? N / Len : FVector(0, 0, 1);
		}

		// UV
		V.UV = (Info.UVIndices[i] != UINT32_MAX)
			? Info.UVs[Info.UVIndices[i]] : FVector2(0, 0);

		V.Color = FVector4(1, 1, 1, 1);
#if IS_OBJ_VIEWER //정점 위치, 노말에 import axis mapping과 import scale을 적용
		V.Position = ApplyImportAxisMapping(V.Position, ActiveImportAxisMapping);
		//여기서 조정된 scale 값을 곱해 저 장합니다.
		V.Position *= ActiveImportAxisMapping.ImportScale;
		V.Normal = ApplyImportAxisMapping(V.Normal, ActiveImportAxisMapping);
#endif

		auto It = UniqueVertices.find(V);
		if (It != UniqueVertices.end())
		{
			// 이미 똑같은 정점이 존재하는 새로 만들지 않고 기존 인덱스만 재사용
			MeshData->Indices.push_back(It->second);
		}
		else
		{
			// 처음 보는 정점이면 배열에 추가하고 해시맵에 인덱스를 기억
			uint32 NewIndex = static_cast<uint32>(MeshData->Vertices.size());
			MeshData->Vertices.push_back(V);
			MeshData->Indices.push_back(NewIndex);
			UniqueVertices[V] = NewIndex;
		}
	}

	// Section 생성
	for (uint32 s = 0; s < Info.SectionStartIndices.size(); ++s)
	{
		FMeshSection Sec;
		Sec.FirstIndex = Info.SectionStartIndices[s];
		Sec.IndexCount = (s + 1 < Info.SectionStartIndices.size())
			? Info.SectionStartIndices[s + 1] - Sec.FirstIndex
			: TriCount - Sec.FirstIndex;
		Sec.MaterialIndex = s;
		MeshData->Sections.push_back(Sec);
	}

	// Section 없으면 전체를 1개로
	if (MeshData->Sections.empty() && !MeshData->Indices.empty())
	{
		FMeshSection Sec;
		Sec.FirstIndex = 0;
		Sec.IndexCount = TriCount;
		Sec.MaterialIndex = 0;
		MeshData->Sections.push_back(Sec);
	}

	MeshData->Topology = EMeshTopology::EMT_TriangleList;
#if IS_OBJ_VIEWER // Viewer에서 오브젝트 중심을 원점으로 옮겨 orbit 회전 중심을 안정화합니다.
	RecenterMeshToOrigin(*MeshData);
#endif
	MeshData->UpdateLocalBound();

	FStaticMeshRenderData* Mesh = new FStaticMeshRenderData();
	Mesh->SetMeshData(MeshData);

	FString Dir = Info.ObjDirectory;

	for (uint32 s = 0; s < MeshData->Sections.size(); ++s)
	{
		FString TexPath = "";
		FVector DiffuseColor(1.0f, 1.0f, 1.0f);
		if (s < Info.MaterialNames.size())
		{
			FString MatName = Info.MaterialNames[s]; // 현재 섹션이 요구하는 머티리얼 이름
			for (const auto& Mtl : Info.Materials)
			{
				if (Mtl.Name == MatName)
				{
					DiffuseColor = Mtl.DiffuseColor;

					if (!Mtl.DiffuseTexturePath.empty())
					{
						FAssetData TexData;
						if (FAssetRegistry::Get().GetAssetByObjectPath(Mtl.DiffuseTexturePath, TexData))
						{
							TexPath = TexData.AssetPath;
						}
						else
						{
							TexPath = Dir + "/" + Mtl.DiffuseTexturePath;
						}
					}
					break; // 머티리얼을 찾았으면 루프 종료
				}
			}
		}
		Mesh->ImportedTexturePaths.push_back(TexPath);
		Mesh->ImportedDiffuseColors.push_back(DiffuseColor);
	}
	return Mesh;
}
