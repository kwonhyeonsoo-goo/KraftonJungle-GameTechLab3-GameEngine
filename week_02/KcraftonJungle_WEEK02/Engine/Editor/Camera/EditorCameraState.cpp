#include "EditorCameraState.h"
#include <cmath>

namespace
{
    constexpr float Pi = 3.14159265358979323846f;
    constexpr float RadToDeg = 180.0f / Pi;
    constexpr float DegToRad = Pi / 180.0f;
    constexpr float TwoPi = 2.0f * Pi;
    constexpr float PitchLimitDeg = 89.0f;

    float WrapAngleRad(float A)
    {
        A = std::fmod(A, TwoPi);
        if (A < 0.0f) A += TwoPi;
        return A;
    }

    float WrapSignedDeg(float A)
    {
        while (A > 180.0f) A -= 360.0f;
        while (A <= -180.0f) A += 360.0f;
        return A;
    }
}

FMatrix FEditorCameraState::GetViewMatrix() const
{
    const FVector Forward = GetForwardVector();
    return FMatrix::LookAt(Position, Position + Forward, FVector(0.0f, 0.0f, 1.0f));
}

FVector FEditorCameraState::GetForwardVector() const
{
    const float CP = std::cos(Pitch);
    const float SP = std::sin(Pitch);
    const float CY = std::cos(Yaw);
    const float SY = std::sin(Yaw);

    return FVector(CP * CY, CP * SY, SP).Normalized();
}

FVector FEditorCameraState::GetRightVector() const
{
    const float CY = std::cos(Yaw);
    const float SY = std::sin(Yaw);

    return FVector(-SY, CY, 0.0f).Normalized();
}

FVector FEditorCameraState::GetUpVector() const
{
    return GetForwardVector().Cross(GetRightVector()).Normalized();
}

float FEditorCameraState::GetYawDeg() const
{
    return WrapSignedDeg(Yaw * RadToDeg);
}

float FEditorCameraState::GetPitchDeg() const
{
    return Pitch * RadToDeg;
}

void FEditorCameraState::SetYawDeg(float Deg)
{
    Yaw = WrapAngleRad(Deg * DegToRad);
}

void FEditorCameraState::SetPitchDeg(float Deg)
{
    if (Deg < -PitchLimitDeg) Deg = -PitchLimitDeg;
    if (Deg > PitchLimitDeg) Deg = PitchLimitDeg;
    Pitch = Deg * DegToRad;
}