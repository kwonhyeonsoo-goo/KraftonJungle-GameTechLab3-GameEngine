#include "StaticMesh.h"

#include <unordered_map>

#include <WICTextureLoader.h>

#include "Graphics/D3D11/D3D11Utils.h"

bool FStaticMesh::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, FStaticMeshSourceData InSourceData)
{
	Release();

	if (InDevice == nullptr || InDeviceContext == nullptr || !InSourceData.IsValid())
	{
		return false;
	}

	SourcePath = std::move(InSourceData.SourcePath);
	BoundsMin = InSourceData.BoundsMin;
	BoundsMax = InSourceData.BoundsMax;
	Vertices = std::move(InSourceData.Vertices);
	Indices = std::move(InSourceData.Indices);

	if (!D3D11Utils::CreateImmutableBuffer(
			InDevice,
			static_cast<UINT>(Vertices.size() * sizeof(FStaticMeshVertex)),
			D3D11_BIND_VERTEX_BUFFER,
			Vertices.data(),
			VertexBuffer)
		|| !D3D11Utils::CreateImmutableBuffer(
			InDevice,
			static_cast<UINT>(Indices.size() * sizeof(uint32)),
			D3D11_BIND_INDEX_BUFFER,
			Indices.data(),
			IndexBuffer))
	{
		Release();
		return false;
	}

	Sections.reserve(InSourceData.Sections.size());
	for (const FStaticMeshSourceData::FSection& SourceSection : InSourceData.Sections)
	{
		Sections.push_back({ SourceSection.IndexStart, SourceSection.IndexCount, SourceSection.MaterialIndex });
	}

	Materials.reserve(InSourceData.Materials.size());
	std::unordered_map<std::string, ID3D11ShaderResourceView*> TextureViewCache;
	for (const FStaticMeshSourceData::FMaterial& SourceMaterial : InSourceData.Materials)
	{
		FMaterial Material = {};
		Material.Name = SourceMaterial.Name;
		Material.DiffuseTexturePath = SourceMaterial.DiffuseTexturePath;

		if (!Material.DiffuseTexturePath.empty())
		{
			const std::string TextureKey = Material.DiffuseTexturePath.lexically_normal().generic_string();
			const auto ExistingTextureIt = TextureViewCache.find(TextureKey);
			if (ExistingTextureIt != TextureViewCache.end())
			{
				Material.DiffuseTextureView = ExistingTextureIt->second;
			}
			else
			{
				DirectX::CreateWICTextureFromFile(
					InDevice,
					InDeviceContext,
					Material.DiffuseTexturePath.c_str(),
					nullptr,
					Material.DiffuseTextureView.GetAddressOf());

				if (Material.DiffuseTextureView != nullptr)
				{
					TextureViewCache.emplace(TextureKey, Material.DiffuseTextureView.Get());
				}
			}
		}

		Materials.push_back(std::move(Material));
	}

	return true;
}

void FStaticMesh::Release()
{
	SourcePath.clear();
	Vertices.clear();
	Indices.clear();
	VertexBuffer.Reset();
	IndexBuffer.Reset();
	Materials.clear();
	Sections.clear();
	SpatialData.reset();

	BoundsMin = FVector::ZeroVector;
	BoundsMax = FVector::ZeroVector;
}
