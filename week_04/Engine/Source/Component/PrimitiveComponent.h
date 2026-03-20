#pragma once
#include "SceneComponent.h"
#include "Primitive/PrimitiveBase.h"
#include "Math/Frustum.h"
#include <memory>
#include <algorithm>
#include <cmath>

class FMaterial;

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

protected:
	std::unique_ptr<CPrimitiveBase> Primitive;
	float LocalBoundRadius = 1.0f;
	FMaterial* Material = nullptr;
};
