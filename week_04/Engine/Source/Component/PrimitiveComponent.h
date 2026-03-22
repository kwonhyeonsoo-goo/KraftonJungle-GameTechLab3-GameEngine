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

	virtual FBoundingSphere GetWorldBounds() const;

	virtual FBoxSphereBounds GetWorldBoundsForAABB() const;

	void UpdateLocalBound();

protected:
	std::shared_ptr<CPrimitiveBase> Primitive;
	FMaterial* Material = nullptr;
};
