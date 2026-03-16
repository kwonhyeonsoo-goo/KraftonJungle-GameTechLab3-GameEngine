#include "Gizmo.h"

void CGizmo::SetMode(EGizmoMode InMode)
{
    Mode = InMode;
}

void CGizmo::CycleMode()
{
    // TODO: Location → Rotation → Scale → Location 순환
}

void CGizmo::Render(/* TODO */)
{
    // TODO: 현재 모드에 따라 기즈모 축 렌더링
}

void CGizmo::OnDragBegin(const FVector& RayOrigin, const FVector& RayDir)
{
    // TODO: 드래그 시작 — 축 선택
}

void CGizmo::OnDragUpdate(const FVector& RayOrigin, const FVector& RayDir)
{
    // TODO: 드래그 중 — Transform 업데이트
}

void CGizmo::OnDragEnd()
{
    ActiveAxis = -1;
}

void CGizmo::SetTargetTransform(const FVector& Position, const FVector& Rotation, const FVector& Scale)
{
    TargetPosition = Position;
    TargetRotation = Rotation;
    TargetScale = Scale;
}
