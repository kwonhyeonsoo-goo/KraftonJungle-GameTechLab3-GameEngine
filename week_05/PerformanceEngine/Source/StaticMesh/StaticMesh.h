#pragma once

#include <filesystem>
#include <memory>

#include "Graphics/D3D11/D3D11Common.h"
#include "Math/Vector.h"
#include "Math/Vector2.h"
#include "Types/Array.h"
#include "Types/PlatformTypes.h"
#include "Types/String.h"

struct FStaticMeshVertex
{
	FVector Position;
	FVector2 TexCoord;
};

struct FStaticMeshSourceData
{
	struct FMaterial
	{
		FString Name;
		std::filesystem::path DiffuseTexturePath;
	};

	struct FSection
	{
		uint32 IndexStart = 0;
		uint32 IndexCount = 0;
		int32 MaterialIndex = -1;
	};

	std::filesystem::path SourcePath;
	TArray<FStaticMeshVertex> Vertices;
	TArray<uint32> Indices;
	TArray<FMaterial> Materials;
	TArray<FSection> Sections;
	FVector BoundsMin = FVector::ZeroVector;
	FVector BoundsMax = FVector::ZeroVector;

	bool IsValid() const { return !Vertices.empty() && !Indices.empty() && !Sections.empty(); }
};

class IStaticMeshSpatialData
{
public:
	virtual ~IStaticMeshSpatialData() = default;
};

class FStaticMesh
{
public:
	struct FMaterial
	{
		FString Name;
		std::filesystem::path DiffuseTexturePath;
		TComPtr<ID3D11ShaderResourceView> DiffuseTextureView;
	};

	struct FSection
	{
		uint32 IndexStart = 0;
		uint32 IndexCount = 0;
		int32 MaterialIndex = -1;
	};

public:
	FStaticMesh() = default;
	~FStaticMesh() = default;

	bool Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, FStaticMeshSourceData InSourceData);
	void Release();

	ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer.Get(); }
	ID3D11Buffer* GetIndexBuffer() const { return IndexBuffer.Get(); }

	UINT GetVertexCount() const { return static_cast<UINT>(Vertices.size()); }
	UINT GetIndexCount() const { return static_cast<UINT>(Indices.size()); }
	bool IsValid() const { return VertexBuffer != nullptr && IndexBuffer != nullptr && !Vertices.empty() && !Indices.empty() && !Sections.empty(); }

	const std::filesystem::path& GetSourcePath() const { return SourcePath; }
	const TArray<FStaticMeshVertex>& GetVertices() const { return Vertices; }
	const TArray<uint32>& GetIndices() const { return Indices; }
	const TArray<FMaterial>& GetMaterials() const { return Materials; }
	const TArray<FSection>& GetSections() const { return Sections; }
	const FVector& GetBoundsMin() const { return BoundsMin; }
	const FVector& GetBoundsMax() const { return BoundsMax; }
	const std::shared_ptr<IStaticMeshSpatialData>& GetSpatialData() const { return SpatialData; }
	void SetSpatialData(std::shared_ptr<IStaticMeshSpatialData> InSpatialData) { SpatialData = std::move(InSpatialData); }

	ID3D11ShaderResourceView* GetMaterialTexture(int32 InMaterialIndex) const
	{
		if (InMaterialIndex < 0 || static_cast<size_t>(InMaterialIndex) >= Materials.size())
		{
			return nullptr;
		}

		return Materials[InMaterialIndex].DiffuseTextureView.Get();
	}

private:
	std::filesystem::path SourcePath;

	TArray<FStaticMeshVertex> Vertices;
	TArray<uint32> Indices;
	TComPtr<ID3D11Buffer> VertexBuffer;
	TComPtr<ID3D11Buffer> IndexBuffer;
	TArray<FMaterial> Materials;
	TArray<FSection> Sections;
	std::shared_ptr<IStaticMeshSpatialData> SpatialData;

	FVector BoundsMin = FVector::ZeroVector;
	FVector BoundsMax = FVector::ZeroVector;
};
