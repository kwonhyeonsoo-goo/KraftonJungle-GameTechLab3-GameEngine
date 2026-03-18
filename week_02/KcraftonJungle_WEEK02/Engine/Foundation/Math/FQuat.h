#pragma once
#include <cmath>
#include "FVector.h"
#include "FMatrix.h"

struct FQuat
{
    float x, y, z, w;

    constexpr FQuat(float _x = 0.f, float _y = 0.f, float _z = 0.f, float _w = 1.f)
        : x(_x), y(_y), z(_z), w(_w)
    {
    }

    static FQuat Identity()
    {
        return FQuat(0.f, 0.f, 0.f, 1.f);
    }

    float LengthSquare() const
    {
        return x * x + y * y + z * z + w * w;
    }

    float LengthSquared() const
    {
        return LengthSquare();
    }

    float Dot(const FQuat& rhs) const
    {
        return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
    }

    FQuat Normalized() const
    {
        const float lenSq = LengthSquared();
        if (lenSq <= 1e-12f)
            return Identity();

        const float invLen = 1.0f / std::sqrt(lenSq);
        return FQuat(x * invLen, y * invLen, z * invLen, w * invLen);
    }

    FQuat Conjugated() const
    {
        return FQuat(-x, -y, -z, w);
    }

    FQuat Inversed() const
    {
        const float lenSq = LengthSquared();
        if (lenSq <= 1e-12f)
            return Identity();

        return FQuat(-x / lenSq, -y / lenSq, -z / lenSq, w / lenSq);
    }

    FQuat operator*(const FQuat& rhs) const
    {
        return FQuat(
            w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
            w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
            w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w,
            w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z
        );
    }

    static FQuat FromAxisAngleRad(const FVector& axis, float angleRad)
    {
        const FVector n = axis.Normalized();
        if (n.IsNearlyZero())
            return Identity();

        const float half = angleRad * 0.5f;
        const float s = std::sin(half);
        const float c = std::cos(half);

        return FQuat(n.x * s, n.y * s, n.z * s, c);
    }

    static FQuat FromEulerRadXYZ(const FVector& eulerRad)
    {
        const float pitch = eulerRad.x;
        const float yaw = eulerRad.y;
        const float roll = eulerRad.z;

        const float hp = pitch * 0.5f;
        const float hy = yaw * 0.5f;
        const float hr = roll * 0.5f;

        const float cp = std::cos(hp);
        const float sp = std::sin(hp);
        const float cy = std::cos(hy);
        const float sy = std::sin(hy);
        const float cr = std::cos(hr);
        const float sr = std::sin(hr);

        const float qw = cp * cy * cr + sp * sy * sr;
        const float qx = sp * cy * cr - cp * sy * sr;
        const float qy = cp * sy * cr + sp * cy * sr;
        const float qz = cp * cy * sr - sp * sy * cr;

        return FQuat(qx, qy, qz, qw).Normalized();
    }

    static FQuat FromEulerDegXYZ(const FVector& eulerDeg)
    {
        constexpr float kPi = 3.14159265358979323846f;
        constexpr float kDegToRad = kPi / 180.0f;
        return FromEulerRadXYZ(eulerDeg * kDegToRad);
    }

    FVector ToEulerDegXYZ() const
    {
        constexpr float kPi = 3.14159265358979323846f;
        constexpr float kRadToDeg = 180.0f / kPi;

        const FQuat q = Normalized();

        const float r00 = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
        const float r22 = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);

        const float sinYaw = 2.0f * (q.w * q.y - q.x * q.z);
        const float sinPitch = 2.0f * (q.y * q.z + q.w * q.x);
        const float sinRoll = 2.0f * (q.x * q.y + q.w * q.z);

        const float yawRad = std::asin((std::min)((std::max)(sinYaw, -1.0f), 1.0f));
        const float pitchRad = std::atan2(sinPitch, r22);
        const float rollRad = std::atan2(sinRoll, r00);

        return FVector(
            pitchRad * kRadToDeg,
            yawRad * kRadToDeg,
            rollRad * kRadToDeg);
    }

    static float NormalizeAngleDeg180(float angleDeg)
    {
        float a = std::fmod(angleDeg + 180.0f, 360.0f);
        if (a < 0.0f)
            a += 360.0f;
        return a - 180.0f;
    }

    static float MakeAngleNearReference(float angleDeg, float referenceDeg)
    {
        const float deltaTurns = std::round((referenceDeg - angleDeg) / 360.0f);
        return angleDeg + 360.0f * deltaTurns;
    }

    FVector ToEulerDegXYZNearest(const FVector& referenceDeg) const
    {
        const FVector base = ToEulerDegXYZ();
        const FVector alt(
            base.x + 180.0f,
            180.0f - base.y,
            base.z + 180.0f);

        auto MakeNear = [&](const FVector& seed) -> FVector
            {
                return FVector(
                    MakeAngleNearReference(seed.x, referenceDeg.x),
                    MakeAngleNearReference(seed.y, referenceDeg.y),
                    MakeAngleNearReference(seed.z, referenceDeg.z));
            };

        const FVector c0 = MakeNear(base);
        const FVector c1 = MakeNear(alt);

        const FVector d0 = c0 - referenceDeg;
        const FVector d1 = c1 - referenceDeg;

        const float score0 = d0.Dot(d0);
        const float score1 = d1.Dot(d1);

        return score0 <= score1 ? c0 : c1;
    }

    FMatrix ToRotationMatrix() const
    {
        const FQuat q = Normalized();

        const float xx = q.x * q.x;
        const float yy = q.y * q.y;
        const float zz = q.z * q.z;
        const float xy = q.x * q.y;
        const float xz = q.x * q.z;
        const float yz = q.y * q.z;
        const float xw = q.x * q.w;
        const float yw = q.y * q.w;
        const float zw = q.z * q.w;

        return FMatrix(
            1.0f - 2.0f * (yy + zz), 2.0f * (xy + zw),        2.0f * (xz - yw),        0.0f,
            2.0f * (xy - zw),        1.0f - 2.0f * (xx + zz), 2.0f * (yz + xw),        0.0f,
            2.0f * (xz + yw),        2.0f * (yz - xw),        1.0f - 2.0f * (xx + yy), 0.0f,
            0.0f,                    0.0f,                    0.0f,                    1.0f);
    }

    FVector RotateVector(const FVector& v) const
    {
        return ToRotationMatrix().TransformVector(v);
    }
};
