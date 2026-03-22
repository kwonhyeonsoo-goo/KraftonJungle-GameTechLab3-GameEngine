#include "PrimitiveComponent.h"
#include "Object/Class.h"
#include "Debug/EngineLog.h"

IMPLEMENT_RTTI(UPrimitiveComponent, USceneComponent)

FBoundingSphere UPrimitiveComponent::GetWorldBounds() const
{
	FMeshData* MeshData = Primitive->GetMeshData();
	FVector Center = GetWorldLocation();
	FVector S = GetWorldTransform().GetScaleVector();
	float MaxScale = (std::max)({ std::abs(S.X), std::abs(S.Y), std::abs(S.Z) });

	if (MeshData)
	{
		Center = GetWorldTransform().TransformPosition(MeshData->GetCenterCoord());
		return { Center, MeshData->GetLocalBoundRadius() * MaxScale };
	}

	/** MeshData 가 없을 때 어떻게 처리할 지는 생각이 필요할듯 */
	UE_LOG("UPrimitiveComponent::GetWorldBounds: MeshData is not found");
	return { Center, MaxScale };
}

FBoxSphereBounds UPrimitiveComponent::GetWorldBoundsForAABB() const
{
	FMeshData* MeshData = Primitive->GetMeshData();
	FVector Center = GetWorldLocation();

	if (MeshData)
	{
		Center = GetWorldTransform().TransformPosition(MeshData->GetCenterCoord());
		FVector S = GetWorldTransform().GetScaleVector();
		FVector ScaledExtent = FVector::Multiply((MeshData->GetMaxCoord() - MeshData->GetMinCoord()) * 0.5, S);
		FMatrix AbsR = FMatrix::Abs(GetWorldTransform().GetRotationMatrix());

		FVector WorldBoxExtent;
		WorldBoxExtent.X =
			AbsR.M[0][0] * ScaledExtent.X +
			AbsR.M[0][1] * ScaledExtent.Y +
			AbsR.M[0][2] * ScaledExtent.Z;

		WorldBoxExtent.Y =
			AbsR.M[1][0] * ScaledExtent.X +
			AbsR.M[1][1] * ScaledExtent.Y +
			AbsR.M[1][2] * ScaledExtent.Z;

		WorldBoxExtent.Z =
			AbsR.M[2][0] * ScaledExtent.X +
			AbsR.M[2][1] * ScaledExtent.Y +
			AbsR.M[2][2] * ScaledExtent.Z;

		return { Center, WorldBoxExtent.SizeSquared(), WorldBoxExtent };
	}

	/** MeshData 가 없을 때 어떻게 처리할 지는 생각이 필요할듯 */
	UE_LOG("UPrimitiveComponent::GetWorldBoundsForAABB: MeshData is not found");
	return { Center, 1, FVector(1, 1, 1) };
}

void UPrimitiveComponent::UpdateLocalBound()
{
	FMeshData* MeshData = Primitive->GetMeshData();

	if (MeshData)
	{
		MeshData->UpdateLocalBound();
	}
}
