#include "StaticMeshComponent.h"
#include "Object/Class.h"
#include "Object/Mesh/StaticMesh.h"
#include "Renderer/Mesh/StaticMeshRenderData.h"
#include "Asset/AssetManager.h"

IMPLEMENT_RTTI(UStaticMeshComponent, UMeshComponent)

void UStaticMeshComponent::SetStaticMesh(UStaticMesh* InMesh)
{
	OverrideMaterials.clear();
	StaticMesh = InMesh;

	FStaticMesh* StaticMeshAsset = StaticMesh->GetAsset();

	/** InMesh 존재 시 OverrideMaterials 크기 조정 및 값 복사 (FStaticMesh 코드 대기중) */
	OverrideMaterials.resize(StaticMeshAsset->MaterialSlotNames.size(), nullptr);//Dorumon_body, Dorumon_eye ...등등 usemtl이 다를 수 있기 때문에

	if (!StaticMeshAsset->MtlPath.empty())
	{
		for (int32 i = 0; i < static_cast<int32>(StaticMeshAsset->MaterialSlotNames.size()); ++i)
		{
			const FString& SlotName = StaticMeshAsset->MaterialSlotNames[i];
			FMaterial* Mat = FAssetManager::GetMaterialByName(SlotName);
			OverrideMaterials[i] = Mat;
		}
	}
}

FBoxSphereBounds UStaticMeshComponent::GetWorldBounds() const
{
	FVector Center = GetWorldLocation();

	if (!Primitive)
	{
		return { Center, 1, FVector(1, 1, 1) };
	}

	FMeshData* MeshData = StaticMesh->GetAsset()->MeshData.get();

	if (MeshData)
	{
		FVector LocalBoxExtent = (MeshData->GetMaxCoord() - MeshData->GetMinCoord()) * 0.5;
		Center = GetWorldTransform().TransformPosition(MeshData->GetCenterCoord());
		FVector S = GetWorldTransform().GetScaleVector();
		FVector ScaledExtent = FVector::Multiply(LocalBoxExtent, S);
		FMatrix AbsR = FMatrix::Abs(GetWorldTransform().GetRotationMatrix());

		FVector WorldBoxExtent;
		// S * R (row-major)
		WorldBoxExtent.X = AbsR.M[0][0] * ScaledExtent.X
			+ AbsR.M[1][0] * ScaledExtent.Y
			+ AbsR.M[2][0] * ScaledExtent.Z;

		WorldBoxExtent.Y = AbsR.M[0][1] * ScaledExtent.X
			+ AbsR.M[1][1] * ScaledExtent.Y
			+ AbsR.M[2][1] * ScaledExtent.Z;

		WorldBoxExtent.Z = AbsR.M[0][2] * ScaledExtent.X
			+ AbsR.M[1][2] * ScaledExtent.Y
			+ AbsR.M[2][2] * ScaledExtent.Z;

		return { Center, WorldBoxExtent.Size(), WorldBoxExtent };
	}

	/** MeshData 가 없을 때 어떻게 처리할 지는 생각이 필요할듯 */
	return { Center, 1, FVector(1, 1, 1) };
}


FMeshData* UStaticMeshComponent::GetMeshData() const
{
	return StaticMesh->GetAsset()->MeshData.get();
}
