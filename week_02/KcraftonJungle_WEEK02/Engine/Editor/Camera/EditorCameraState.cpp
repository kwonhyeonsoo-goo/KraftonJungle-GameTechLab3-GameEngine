#include "EditorCameraState.h"
#include <cmath>

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