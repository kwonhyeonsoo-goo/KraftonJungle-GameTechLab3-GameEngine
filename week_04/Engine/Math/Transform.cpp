#include "Math/Transform.h"

FMatrix FTransform::ToMatrix() const
{
	FMatrix S = FMatrix::MakeScale(Scale);
	FMatrix R = FMatrix::MakeRotationZ(Rotation.Z) * FMatrix::MakeRotationY(Rotation.Y) * FMatrix::MakeRotationX(Rotation.X);
	FMatrix T = FMatrix::MakeTranslation(Location);
	return S * R * T;
}
