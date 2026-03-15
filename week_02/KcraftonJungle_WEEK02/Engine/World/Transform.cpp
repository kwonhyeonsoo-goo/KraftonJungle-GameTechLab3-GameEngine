#include "Transform.h"

namespace
{
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kDegToRad = kPi / 180.0f;
}

Transform::Transform() : Location(FVector::Zero), Rotation(FVector::Zero), Scale(FVector::One)
{
}

Transform::Transform(const FVector& loc, const FVector& rot, const FVector& scale)
{
    Location = loc;
    Rotation = rot;
    Scale = scale;
}

FMatrix Transform::ToMatrix() const
{
    const float pitch = Rotation.x * kDegToRad;
    const float yaw = Rotation.y * kDegToRad;
    const float roll = Rotation.z * kDegToRad;

    return
        FMatrix::Scale(Scale) *
        FMatrix::RotationX(pitch) *
        FMatrix::RotationY(-yaw) *
        FMatrix::RotationZ(roll) *
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

FVector Transform::GetRotation() const
{
    return Rotation;
}

FVector Transform::GetScale() const
{
    return Scale;
}