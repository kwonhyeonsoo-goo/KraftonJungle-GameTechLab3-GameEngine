#include "PrimitiveComponent.h"
#include "Object/Class.h"

IMPLEMENT_RTTI(UPrimitiveComponent, USceneComponent)

FBoundingSphere UPrimitiveComponent::GetWorldBounds() const
{
	FVector Center = GetWorldLocation();
	FTransform T = GetRelativeTransform();
	FVector S = T.GetScale3D();
	float MaxScale = (std::max)({ std::abs(S.X), std::abs(S.Y), std::abs(S.Z) });
	return { Center, LocalBoundRadius * MaxScale };
}

FBoxSphereBounds UPrimitiveComponent::GetWorldBoundsForAABB() const
{
	FVector Center = GetWorldLocation();
	FVector S = GetWorldTransform().GetScaleVector();
	FVector ScaledExtent = FVector::Multiply(LocalBoxExtent, S);
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