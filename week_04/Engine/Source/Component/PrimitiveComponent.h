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
	static UClass* StaticClass();

	UPrimitiveComponent() : USceneComponent(StaticClass(), "") {}

	UPrimitiveComponent(UClass* InClass, const FString& InName, UObject* InOuter = nullptr)
		: USceneComponent(InClass, InName, InOuter) {}

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

		FMatrix AbsR = FMatrix::Abs(T.GetRotation().ToMatrix());		
		FVector NewS(FVector::DotProduct(AbsR.GetUnitAxis(EAxis::X), S), FVector::DotProduct(AbsR.GetUnitAxis(EAxis::Y), S), FVector::DotProduct(AbsR.GetUnitAxis(EAxis::Z), S));

		FVector WorldBoxExtent = FVector::Multiply(LocalBoxExtent, NewS);

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

	
	FVector LocalBoxExtent = FVector(0.5, 0.5, 0.5);
};
