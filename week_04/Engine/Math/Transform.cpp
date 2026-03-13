#include "Math/Transform.h"

FMatrix FTransform::ToMatrix() const
{
	FMatrix S = FMatrix::Scale(Scale.X, Scale.Y, Scale.Z);
	FMatrix R = FMatrix::RotationZ(Rotation.Z) * FMatrix::RotationY(Rotation.Y) * FMatrix::RotationX(Rotation.X);
	FMatrix T = FMatrix::Translation(Location.X, Location.Y, Location.Z);
	return S * R * T;
}
