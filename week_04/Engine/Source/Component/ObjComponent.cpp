#include "ObjComponent.h"
#include "Primitive/PrimitiveObj.h"
#include "Object/Class.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/Material.h"
#include "Renderer/ShaderMap.h"
#include "Renderer/PrimitiveVertex.h"
#include "Core/Paths.h"
#include "Asset/AssetManager.h"
#include "Renderer/Mesh/StaticMeshRenderData.h"
#include "ThirdParty/stb_image.h"
#include <d3d11.h>

// FStaticMesh 데이터를 FMeshData(GPU 버퍼)로 변환하는 헬퍼
class CPrimitiveStaticMesh : public CPrimitiveBase
{
public:
	bool Init(const FStaticMesh* InStaticMesh, ID3D11Device* Device)
	{
		MeshData = std::make_shared<FMeshData>();

		MeshData->Vertices.reserve(InStaticMesh->Vertices.size());
		for (const FNormalVertex& NV : InStaticMesh->Vertices)
		{
			FPrimitiveVertex PV;
			PV.Position = NV.Position;
			PV.Color = NV.Color;
			PV.Normal = NV.Normal;
			PV.UV = NV.UV;
			MeshData->Vertices.push_back(PV);
		}

		MeshData->Indices.assign(InStaticMesh->Indices.begin(), InStaticMesh->Indices.end());
		MeshData->Topology = EMeshTopology::EMT_TriangleList;

		return MeshData->CreateVertexAndIndexBuffer(Device);
	}
};

IMPLEMENT_RTTI(UObjComponent, UPrimitiveComponent)

void UObjComponent::Initialize()
{
}

void UObjComponent::LoadStaticMeshAsset(ID3D11Device* Device, const FString& FilePath)
{
	// 1. FStaticMesh 로드 (캐시 히트 시 재사용)
	StaticMesh = FAssetManager::LoadObjStaticMeshAsset(FilePath, Device);
	if (!StaticMesh)
	{
		printf("[ObjComponent] Failed to load static mesh: %s\n", FilePath.c_str());
		return;
	}

	// 2. FStaticMesh → FMeshData (GPU 버퍼 생성)
	auto PrimitiveSM = std::make_shared<CPrimitiveStaticMesh>();
	if (!PrimitiveSM->Init(StaticMesh, Device))
	{
		printf("[ObjComponent] Failed to create GPU buffers: %s\n", FilePath.c_str());
		return;
	}
	Primitive = PrimitiveSM;

	// 3. MTL 머티리얼 로드 → 슬롯별 매핑
	MaterialSlots.clear();
	MaterialSlots.resize(StaticMesh->MaterialSlotNames.size(), nullptr);//Dorumon_body, Dorumon_eye ...등등 usemtl이 다를 수 있기 때문에

	if (!StaticMesh->MtlPath.empty())
	{
		for (int32 i = 0; i < static_cast<int32>(StaticMesh->MaterialSlotNames.size()); ++i)
		{
			const FString& SlotName = StaticMesh->MaterialSlotNames[i];
			FMaterial* Mat = FAssetManager::GetMaterialByName(SlotName);
			MaterialSlots[i] = Mat;
		}
	}
}

FMaterial* UObjComponent::GetMaterialBySlot(int32 SlotIndex) const
{
	if (SlotIndex >= 0 && SlotIndex < static_cast<int32>(MaterialSlots.size()))
		return MaterialSlots[SlotIndex];
	return nullptr;
}
