#pragma once

#include "../../Foundation/Math/FVector.h"
#include "../../Foundation/Math/FMatrix.h"

struct FEditorCameraState
{
    FVector Position = FVector(-10.0f, 0.0f, 0.0f);

    float Yaw = 0.0f;
    float Pitch = 0.0f;

    float MoveSpeed = 5.0f;
    float RotSpeed = 0.3f;

    FMatrix GetViewMatrix() const;
    FVector GetForwardVector() const;
    FVector GetRightVector() const;
    FVector GetUpVector() const;
    float GetYawDeg() const;
    float GetPitchDeg() const;
    void SetYawDeg(float Deg);
    void SetPitchDeg(float Deg);
};