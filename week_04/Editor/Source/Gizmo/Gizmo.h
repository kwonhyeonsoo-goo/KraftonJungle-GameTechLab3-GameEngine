#pragma once

#include "Math/Vector.h"

enum class EGizmoMode
{
    Location,
    Rotation,
    Scale
};

class CGizmo
{
public:
    void SetMode(EGizmoMode InMode);
    EGizmoMode GetMode() const { return Mode; }
    void CycleMode(); // Space Bar 토글

    void Render(/* TODO: 렌더러 파라미터 */);
    void OnDragBegin(const FVector& RayOrigin, const FVector& RayDir);
    void OnDragUpdate(const FVector& RayOrigin, const FVector& RayDir);
    void OnDragEnd();

    void SetTargetTransform(const FVector& Position, const FVector& Rotation, const FVector& Scale);

private:
    EGizmoMode Mode = EGizmoMode::Location;
    FVector TargetPosition;
    FVector TargetRotation;
    FVector TargetScale = { 1.0f, 1.0f, 1.0f };
    int32 ActiveAxis = -1; // 0=X, 1=Y, 2=Z, -1=None
};
