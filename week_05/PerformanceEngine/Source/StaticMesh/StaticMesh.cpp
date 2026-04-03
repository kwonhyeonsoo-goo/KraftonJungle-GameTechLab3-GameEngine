#include "StaticMesh.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <WICTextureLoader.h>

namespace
{
	struct FObjFaceIndex
	{
		int Position = 0;
		int TexCoord = 0;
	};

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

	bool StartsWith(const std::string& InValue, const char* Prefix)
	{
		return InValue.rfind(Prefix, 0) == 0;
	}

	bool ParseFaceIndex(const std::string& InToken, FObjFaceIndex& OutIndex)
	{
		int PositionIndex = 0;
		int TexCoordIndex = 0;
		if (::sscanf_s(InToken.c_str(), "%d/%d/%*d", &PositionIndex, &TexCoordIndex) != 2)
		{
			return false;
		}

		OutIndex.Position = PositionIndex;
		OutIndex.TexCoord = TexCoordIndex;
		return true;
	}

	std::filesystem::path LoadDiffuseTexturePath(const std::filesystem::path& InMaterialPath)
	{
		if (InMaterialPath.empty())
		{
			return {};
		}

		std::ifstream MaterialFile(InMaterialPath);
		if (!MaterialFile)
		{
			return {};
		}

		std::string Line;
		while (std::getline(MaterialFile, Line))
		{
			const std::string TrimmedLine = Trim(Line);
			if (!StartsWith(TrimmedLine, "map_Kd "))
			{
				continue;
			}

			const std::string RelativeTexturePath = Trim(TrimmedLine.substr(7));
			if (RelativeTexturePath.empty())
			{
				return {};
			}

			return InMaterialPath.parent_path() / std::filesystem::path(RelativeTexturePath);
		}

		return {};
	}
}

bool FStaticMesh::LoadFromObj(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, const std::filesystem::path& InObjPath)
{
	Release();

	if (InDevice == nullptr || InDeviceContext == nullptr || InObjPath.empty())
	{
		return false;
	}

	std::ifstream ObjFile(InObjPath);
	if (!ObjFile)
	{
		return false;
	}

	SourcePath = InObjPath;

	TArray<FVector> Positions;
	TArray<FVector2> TexCoords;
	Positions.reserve(2048);
	TexCoords.reserve(2048);

	std::filesystem::path MaterialPath;

	BoundsMin = FVector(
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max());
	BoundsMax = FVector(
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest());

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
			MaterialPath = InObjPath.parent_path() / std::filesystem::path(RelativeMaterialPath);
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

		auto EmitVertex = [&](const FObjFaceIndex& InFaceIndex)
		{
			const size_t PositionIndex = static_cast<size_t>(std::max(InFaceIndex.Position - 1, 0));
			const size_t TexCoordIndex = static_cast<size_t>(std::max(InFaceIndex.TexCoord - 1, 0));

			if (PositionIndex >= Positions.size())
			{
				return;
			}

			const FVector Position = Positions[PositionIndex];
			const FVector2 TexCoord = TexCoordIndex < TexCoords.size() ? TexCoords[TexCoordIndex] : FVector2();

			Vertices.push_back({ Position, TexCoord });
			ExpandBounds(Position);
		};

		for (size_t TriangleIndex = 1; TriangleIndex + 1 < FaceIndices.size(); ++TriangleIndex)
		{
			EmitVertex(FaceIndices[0]);
			EmitVertex(FaceIndices[TriangleIndex]);
			EmitVertex(FaceIndices[TriangleIndex + 1]);
		}
	}

	if (Vertices.empty())
	{
		Release();
		return false;
	}

	const D3D11_BUFFER_DESC VertexBufferDesc = {
		static_cast<UINT>(Vertices.size() * sizeof(FStaticMeshVertex)),
		D3D11_USAGE_IMMUTABLE,
		D3D11_BIND_VERTEX_BUFFER,
		0,
		0,
		0
	};

	const D3D11_SUBRESOURCE_DATA InitialVertexData = {
		Vertices.data(),
		0,
		0
	};

	if (FAILED(InDevice->CreateBuffer(&VertexBufferDesc, &InitialVertexData, VertexBuffer.GetAddressOf())))
	{
		Release();
		return false;
	}

	const std::filesystem::path DiffuseTexturePath = LoadDiffuseTexturePath(MaterialPath);
	if (!DiffuseTexturePath.empty())
	{
		DirectX::CreateWICTextureFromFile(
			InDevice,
			InDeviceContext,
			DiffuseTexturePath.c_str(),
			nullptr,
			DiffuseTextureView.GetAddressOf());
	}

	return true;
}

void FStaticMesh::Release()
{
	SourcePath.clear();
	Vertices.clear();
	VertexBuffer.Reset();
	DiffuseTextureView.Reset();
	SpatialData.reset();

	BoundsMin = FVector::ZeroVector;
	BoundsMax = FVector::ZeroVector;
}

void FStaticMesh::ExpandBounds(const FVector& InPosition)
{
	BoundsMin.X = std::min(BoundsMin.X, InPosition.X);
	BoundsMin.Y = std::min(BoundsMin.Y, InPosition.Y);
	BoundsMin.Z = std::min(BoundsMin.Z, InPosition.Z);

	BoundsMax.X = std::max(BoundsMax.X, InPosition.X);
	BoundsMax.Y = std::max(BoundsMax.Y, InPosition.Y);
	BoundsMax.Z = std::max(BoundsMax.Z, InPosition.Z);
}
