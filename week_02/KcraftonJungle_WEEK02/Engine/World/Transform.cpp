#include "Transform.h"

namespace
{
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kDegToRad = kPi / 180.0f;
    constexpr float kRadToDeg = 180.0f / kPi;
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
        FMatrix::RotationY(yaw) *
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

// (x, y, z, w)
FVector4 Transform::ToQuaternion() const
{
    const float pitch = Rotation.x * kDegToRad;
    const float yaw = Rotation.y * kDegToRad;
    const float roll = Rotation.z * kDegToRad;

    const float hp = pitch * 0.5f;
    const float hy = yaw * 0.5f;
    const float hr = roll * 0.5f;

    const float cp = std::cos(hp);
    const float sp = std::sin(hp);
    const float cy = std::cos(hy);
    const float sy = std::sin(hy);
    const float cr = std::cos(hr);
    const float sr = std::sin(hr);

    const float w = cp * cy * cr + sp * sy * sr;
    const float x = sp * cy * cr - cp * sy * sr;
    const float y = cp * sy * cr + sp * cy * sr;
    const float z = cp * cy * sr - sp * sy * cr;

    return FVector4(x, y, z, w);
}

FVector Transform::ToEularAngle(const FVector4& Quaternion)
{
    const float LenSq = Quaternion.LengthSquared();

    if (LenSq <= 0.0f) return FVector::Zero;

    const float invLen = 1.0f / std::sqrt(LenSq);

    const float x = Quaternion.x * invLen;
    const float y = Quaternion.y * invLen;
    const float z = Quaternion.z * invLen;
    const float w = Quaternion.w * invLen;

    const float r00 = 1.0f - 2.0f * (y * y + z * z);
    const float r22 = 1.0f - 2.0f * (x * x + y * y);

    const float sinYaw   = 2.0f * (w * y - x * z);
    const float sinPitch = 2.0f * (y * z + w * x);
    const float sinRoll  = 2.0f * (x * y + w * z);

    const float yawRad   = std::asin((std::min)((std::max)(sinYaw, -1.0f), 1.0f));
    const float pitchRad = std::atan2(sinPitch, r22);
    const float rollRad  = std::atan2(sinRoll, r00);

    return FVector(
        pitchRad * kRadToDeg,
        yawRad * kRadToDeg,
        rollRad * kRadToDeg
    );
}
