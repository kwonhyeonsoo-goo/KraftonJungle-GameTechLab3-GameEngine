#include "Transform.h"

Transform::Transform()
    : Location(FVector::Zero)
    , Rotation(FQuat::Identity())
    , Scale(FVector::One)
{
}

Transform::Transform(const FVector& loc, const FQuat& rot, const FVector& scale)
    : Location(loc)
    , Rotation(rot.Normalized())
    , Scale(scale)
{
}

Transform Transform::FromEulerDegrees(const FVector& loc,
    const FVector& rotDeg,
    const FVector& scale)
{
    return Transform(loc, FQuat::FromEulerDegXYZ(rotDeg), scale);
}

FMatrix Transform::ToMatrix() const
{
    return
        FMatrix::Scale(Scale) *
        Rotation.ToRotationMatrix() *
        FMatrix::Translation(Location);
}

FMatrix Transform::ToInverseMatrix() const
{
    return ToMatrix().Inversed();
}

FVector Transform::GetLocation() const
{
    return Location;
}

FQuat Transform::GetRotation() const
{
    return Rotation;
}

FVector Transform::GetScale() const
{
    return Scale;
}

FVector Transform::GetEulerDegrees() const
{
    return Rotation.ToEulerDegXYZ();
}

FVector Transform::GetEulerDegreesNearest(const FVector& referenceDeg) const
{
    return Rotation.ToEulerDegXYZNearest(referenceDeg);
}

void Transform::SetEulerDegrees(const FVector& eulerDeg)
{
    Rotation = FQuat::FromEulerDegXYZ(eulerDeg);
}
