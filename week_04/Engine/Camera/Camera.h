#pragma once

#include "Math/Vector.h"
#include "Math/Matrix.h"

class CCamera
{
public:
    CCamera() = default;

    void SetPosition(const FVector& InPosition);
    void SetLookAt(const FVector& InTarget);

    // 입력 처리
    void MoveForward(float Delta);
    void MoveRight(float Delta);
    void Rotate(float DeltaYaw, float DeltaPitch);

    FMatrix GetViewMatrix() const;
    FMatrix GetProjectionMatrix() const;

    void SetAspectRatio(float InAspectRatio);

private:
    FVector Position = { 0.0f, 5.0f, -10.0f };
    FVector Target = { 0.0f, 0.0f, 0.0f };
    FVector Up = { 0.0f, 1.0f, 0.0f };

    float Yaw = 0.0f;
    float Pitch = 0.0f;

    float FOV = 45.0f;
    float AspectRatio = 16.0f / 9.0f;
    float NearPlane = 0.1f;
    float FarPlane = 1000.0f;
};
