#include "PrimitiveObj.h"
#include <fstream>
#include <sstream>

CPrimitiveObj::CPrimitiveObj()
{
	MeshData = std::make_shared<FMeshData>();
}

CPrimitiveObj::CPrimitiveObj(const FString& FilePath)
{
	MeshData = std::make_shared<FMeshData>();
	LoadObj(FilePath);
}

void CPrimitiveObj::LoadObj(const FString& FilePath)
{
	SetPrimitiveFileName(FilePath);
	auto Cached = GetCached(FilePath);
	if (Cached)
	{
		MeshData = Cached;
		return;
	}

	std::ifstream File(FilePath);
	if (!File.is_open())
	{
		printf("[OBJ] Failed to open: %s\n", FilePath.c_str());
		return;
	}
	std::vector<FVector> Positions;
	std::string Line;

	while (std::getline(File, Line))
	{
		std::stringstream SS(Line);
		std::string Type;
		SS >> Type;

		// vertex
		if (Type == "v")
		{
			FVector Pos;
			SS >> Pos.X >> Pos.Y >> Pos.Z;
			Positions.push_back(Pos);
		}
		// face
		else if (Type == "f")
		{
			std::string VStr;
			std::vector<uint32> FaceIndices;

			while (SS >> VStr)
			{
				std::stringstream VSS(VStr);
				std::string IndexStr;
				std::getline(VSS, IndexStr, '/');

				uint32 Index = std::stoi(IndexStr) - 1; // OBJ는 1-based
				FaceIndices.push_back(Index);
			}

			// 삼각형 기준 (fan triangulation)
			for (size_t i = 1; i + 1 < FaceIndices.size(); ++i)
			{
				uint32 i0 = FaceIndices[0];
				uint32 i1 = FaceIndices[i];
				uint32 i2 = FaceIndices[i + 1];

				MeshData->Indices.push_back(i0);
				MeshData->Indices.push_back(i1);
				MeshData->Indices.push_back(i2);
			}
		}
	}

	// Vertex 생성
	MeshData->Vertices.resize(Positions.size());

	for (size_t i = 0; i < Positions.size(); ++i)
	{
		MeshData->Vertices[i].Position = Positions[i];
		MeshData->Vertices[i].Color = FVector4(Positions[i].X, Positions[i].Y, Positions[i].Z, 1);
	}

	MeshData->Topology = EMeshTopology::EMT_TriangleList;
	RegisterMeshData(FilePath, MeshData);
}