#pragma once
#include "SceneComponent.h"
#include "Primitive/PrimitiveBase.h"
#include "Math/Frustum.h"
#include <memory>
#include <algorithm>
#include <cmath>

class FMaterial;

struct FBoxSphereBounds
{
	FVector Center;
	float RadiusSquared;
	FVector BoxExtent;
};

class ENGINE_API UPrimitiveComponent : public USceneComponent
{
public:
	DECLARE_RTTI(UPrimitiveComponent, USceneComponent)

	CPrimitiveBase* GetPrimitive() const { return Primitive.get(); }

	void SetMaterial(FMaterial* InMaterial) { Material = InMaterial; }
	FMaterial* GetMaterial() const { return Material; }

	FBoundingSphere GetWorldBounds() const
	{
		FVector Center = GetWorldLocation();
		FTransform T = GetRelativeTransform();
		FVector S = T.GetScale3D();
		float MaxScale = (std::max)({ std::abs(S.X), std::abs(S.Y), std::abs(S.Z) });
		return { Center, LocalBoundRadius * MaxScale };
	}

	virtual FBoxSphereBounds GetWorldBoundsForAABB() const
	{
		FVector Center = GetWorldLocation();
		FTransform T = GetRelativeTransform();
		FVector S = T.GetScale3D();

		FVector ScaledExtent = FVector::Multiply(LocalBoxExtent, S);

		FMatrix AbsR = FMatrix::Abs(T.GetRotation().ToMatrix());

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

		return { Center, WorldBoxExtent.SizeSquared(), WorldBoxExtent};
	}

	FVector GetLocalBoxExtent() const
	{
		return LocalBoxExtent;
	}

protected:
	std::unique_ptr<CPrimitiveBase> Primitive;
	float LocalBoundRadius = 1.0f;
	FMaterial* Material = nullptr;

	
	FVector LocalBoxExtent = FVector(0.75, 0.75, 0.75);
};
