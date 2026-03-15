#pragma once
#include "SceneComponent.h"
#include "Primitive/PrimitiveBase.h"
#include "Math/Frustum.h"
#include <memory>
#include <algorithm>
#include <cmath>

class ENGINE_API UPrimitiveComponent : public USceneComponent
{
public:
	static UClass* StaticClass();

	UPrimitiveComponent() : USceneComponent(StaticClass(), "") {}

	UPrimitiveComponent(UClass* InClass, const FString& InName, UObject* InOuter = nullptr)
		: USceneComponent(InClass, InName, InOuter) {}

	CPrimitiveBase* GetPrimitive() const { return Primitive.get(); }

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
};
