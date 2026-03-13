#include "Camera.h"

void CCamera::SetPosition(const FVector& InPosition)
{
    Position = InPosition;
}

void CCamera::SetLookAt(const FVector& InTarget)
{
    Target = InTarget;
}

void CCamera::MoveForward(float Delta)
{
    // TODO: 카메라 전후 이동 (W/S)
}

void CCamera::MoveRight(float Delta)
{
    // TODO: 카메라 좌우 이동 (A/D)
}

void CCamera::Rotate(float DeltaYaw, float DeltaPitch)
{
    // TODO: 마우스 우클릭 드래그 → Yaw/Pitch 회전
}

FMatrix CCamera::GetViewMatrix() const
{
    return FMatrix::LookAt(Position, Target, Up);
}

FMatrix CCamera::GetProjectionMatrix() const
{
    return FMatrix::Perspective(FOV, AspectRatio, NearPlane, FarPlane);
}

void CCamera::SetAspectRatio(float InAspectRatio)
{
    AspectRatio = InAspectRatio;
}
