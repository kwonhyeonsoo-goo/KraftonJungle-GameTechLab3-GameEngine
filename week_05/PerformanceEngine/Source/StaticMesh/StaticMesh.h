#pragma once

#include <filesystem>
#include <memory>

#include "Graphics/D3D11/D3D11Common.h"
#include "Math/Vector.h"
#include "Math/Vector2.h"
#include "Types/Array.h"

struct FStaticMeshVertex
{
	FVector Position;
	FVector2 TexCoord;
};

class IStaticMeshSpatialData
{
public:
	virtual ~IStaticMeshSpatialData() = default;
};

class FStaticMesh
{
public:
	FStaticMesh() = default;
	~FStaticMesh() = default;

	bool LoadFromObj(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, const std::filesystem::path& InObjPath);
	void Release();

	ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer.Get(); }
	ID3D11ShaderResourceView* GetDiffuseTexture() const { return DiffuseTextureView.Get(); }

	UINT GetVertexCount() const { return static_cast<UINT>(Vertices.size()); }
	bool IsValid() const { return VertexBuffer != nullptr && !Vertices.empty(); }

	const std::filesystem::path& GetSourcePath() const { return SourcePath; }
	const TArray<FStaticMeshVertex>& GetVertices() const { return Vertices; }
	const FVector& GetBoundsMin() const { return BoundsMin; }
	const FVector& GetBoundsMax() const { return BoundsMax; }
	const std::shared_ptr<IStaticMeshSpatialData>& GetSpatialData() const { return SpatialData; }
	void SetSpatialData(std::shared_ptr<IStaticMeshSpatialData> InSpatialData) { SpatialData = std::move(InSpatialData); }

private:
	void ExpandBounds(const FVector& InPosition);

private:
	std::filesystem::path SourcePath;

	TArray<FStaticMeshVertex> Vertices;
	TComPtr<ID3D11Buffer> VertexBuffer;
	TComPtr<ID3D11ShaderResourceView> DiffuseTextureView;
	std::shared_ptr<IStaticMeshSpatialData> SpatialData;

	FVector BoundsMin = FVector::ZeroVector;
	FVector BoundsMax = FVector::ZeroVector;
};
