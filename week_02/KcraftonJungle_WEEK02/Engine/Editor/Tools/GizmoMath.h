#pragma once
#include "ITool.h"
#include "../../Foundation/Math/FVector2D.h"
#include "../../Foundation/Math/FVector4.h"
#include "../../Services/PickingService.h"
#include "../../Editor/ToolContext.h"
#include "../../World/USceneComponent.h"
#include "../../World/Transform.h"
#include "../../../AppContext.h"

namespace GizmoMath
{
    constexpr float  GizmoAxisLength    = 1.5f;
    constexpr float  GizmoRingRadius    = 1.0f;  // Torus.h R=1.0 에 맞춤
    constexpr float  GizmoPickThresholdPx = 30.0f;
    constexpr float  GizmoScaleBoxPx   = 60.0f;
    constexpr float  kGizmoScreenScale  = 0.1f;  // dist * kGizmoScreenScale = 월드 스케일

    inline float ComputeGizmoScale(const FVector& gizmoPos, const FVector& cameraPos)
    {
        return (gizmoPos - cameraPos).Length() * kGizmoScreenScale;
    }

    constexpr uint32 AxisColorX = 0xFF0000FF;
    constexpr uint32 AxisColorY = 0x00FF00FF;
    constexpr uint32 AxisColorZ = 0x0000FFFF;

    FVector AxisX();
    FVector AxisY();
    FVector AxisZ();

    FVector2D WorldToScreen(const FVector& worldPos,
        const FMatrix& viewProj,
        int32 viewportW, int32 viewportH);

    FVector GetAxisDirection(const USceneComponent* comp,
        int axisIndex,
        ECoordSpace coordSpace);

    Ray BuildMouseRay(const MouseEvent& e, AppContext& ctx);

    bool NearlySameLocation(const FVector& a, const FVector& b, float eps = 0.0001f);

    FVector GetSnapAppliedPosition(const FVector& origin,
        const FVector& axisDir,
        float axisDistance,
        bool snapEnabled,
        float snapValue);

    float DistancePointToSegment2D(const FVector2D& mouse,
        const FVector2D& segA,
        const FVector2D& segB);

    float DistancePointToPolyline2D(const FVector2D& mouse,
        const TArray<FVector2D>& points);

    TArray<FVector2D> SampleRing2D(const FVector& center,
        const FVector& normal,
        float radius,
        int32 samples,
        const FMatrix& viewProj,
        int32 viewportW, int32 viewportH);

    bool PointInRect2D(const FVector2D& mouse,
        const FVector2D& center,
        float halfSize);

    bool RayPlaneIntersection(const Ray& ray,
        const FVector& planeOrigin,
        const FVector& planeNormal,
        FVector& outHit);

    // �ٽ�: ���콺 ray�� gizmo �� ������ �ִ������� �� �Ķ���͸� ���Ѵ�.
    // outAxisT�� axisOrigin + axisDir * outAxisT �� t���̴�.
    bool ClosestAxisParameterToRay(const FVector& axisOrigin,
        const FVector& axisDir,
        const Ray& ray,
        float& outAxisT);

    float SignedAngleAroundAxis(const FVector& fromDir,
        const FVector& toDir,
        const FVector& axis);

    FVector MakeStablePerpendicular(const FVector& dir);

    FMatrix MakeAxisTransform(const FVector& basePos, const FVector& axisDir, float scale);
}