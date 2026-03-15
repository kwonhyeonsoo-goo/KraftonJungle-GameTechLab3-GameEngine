#include "GizmoMath.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr float PI = 3.14159265358979323846f;

    float DegToRad(float degrees)
    {
        return degrees * (PI / 180.0f);
    }

    float ClampFloat(float v, float lo, float hi)
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    FMatrix BuildRotationMatrixFromDegrees(const FVector& rotationDeg)
    {
        return FMatrix::RotationX(DegToRad(rotationDeg.x))
            * FMatrix::RotationY(-DegToRad(rotationDeg.y))
            * FMatrix::RotationZ(DegToRad(rotationDeg.z));
    }

    FVector TransformDirection(const FVector& dir, const FMatrix& rot)
    {
        FVector4 v(dir, 0.0f);
        FVector4 r = v * rot;
        return FVector(r.x, r.y, r.z).Normalized();
    }

    float Dot2(const FVector2D& a, const FVector2D& b)
    {
        return a.x * b.x + a.y * b.y;
    }
}

FMatrix GizmoMath::MakeAxisTransform(const FVector& basePos, const FVector& axisDir, float scale)
{
    FVector localZ = axisDir.Normalized();
    FVector localX = MakeStablePerpendicular(localZ);
    FVector localY = localZ.Cross(localX).Normalized();

    return {
        localX.x * scale, localX.y * scale, localX.z * scale, 0.0f,
        localY.x * scale, localY.y * scale, localY.z * scale, 0.0f,
        localZ.x * scale, localZ.y * scale, localZ.z * scale, 0.0f,
        basePos.x,        basePos.y,        basePos.z,        1.0f
    };
}

FVector GizmoMath::AxisX()
{
    return FVector(1.f, 0.f, 0.f);
}

FVector GizmoMath::AxisY()
{
    return FVector(0.f, 1.f, 0.f);
}

FVector GizmoMath::AxisZ()
{
    return FVector(0.f, 0.f, 1.f);
}

FVector2D GizmoMath::WorldToScreen(const FVector& worldPos,
    const FMatrix& viewProj,
    int32 viewportW, int32 viewportH)
{
    const FVector4 clip = FVector4(worldPos, 1.0f) * viewProj;

    constexpr float eps = 1e-6f;
    if (clip.w < eps)
    {
        return FVector2D((std::numeric_limits<float>::max)(),
            (std::numeric_limits<float>::max)());
    }

    const float invW = 1.0f / clip.w;
    const float ndcX = clip.x * invW;
    const float ndcY = clip.y * invW;

    const float screenX = (ndcX + 1.0f) * 0.5f * static_cast<float>(viewportW);
    const float screenY = (1.0f - ndcY) * 0.5f * static_cast<float>(viewportH);

    return FVector2D(screenX, screenY);
}

FVector GizmoMath::GetAxisDirection(const USceneComponent* comp,
    int axisIndex,
    ECoordSpace coordSpace)
{
    const FVector basis =
        axisIndex == 0 ? AxisX() :
        axisIndex == 1 ? AxisY() :
        AxisZ();

    if (coordSpace == ECoordSpace::World || comp == nullptr)
        return basis;

    const Transform t = comp->GetTransform();
    const FMatrix rot = BuildRotationMatrixFromDegrees(t.Rotation);
    return TransformDirection(basis, rot);
}

Ray GizmoMath::BuildMouseRay(const MouseEvent& e, AppContext& ctx)
{
    const FMatrix viewProj = ctx.Editor.GetViewProjMatrix();
    const FMatrix invViewProj = viewProj.Inversed();

    return PickingService::ScreenToRay(
        e.X, e.Y,
        ctx.Window.GetWidth(),
        ctx.Window.GetHeight(),
        invViewProj);
}

bool GizmoMath::NearlySameLocation(const FVector& a, const FVector& b, float eps)
{
    const FVector d = a - b;
    return d.Dot(d) <= eps * eps;
}

FVector GizmoMath::GetSnapAppliedPosition(const FVector& origin,
    const FVector& axisDir,
    float axisDistance,
    bool snapEnabled,
    float snapValue)
{
    float d = axisDistance;

    if (snapEnabled && snapValue > 0.0001f)
    {
        d = std::round(d / snapValue) * snapValue;
    }

    return origin + axisDir * d;
}

float GizmoMath::DistancePointToSegment2D(const FVector2D& mouse,
    const FVector2D& segA,
    const FVector2D& segB)
{
    const FVector2D ab = segB - segA;
    const FVector2D ap = mouse - segA;
    const float abLenSq = Dot2(ab, ab);

    constexpr float eps = 1e-6f;
    if (abLenSq < eps)
        return (mouse - segA).Length();

    float t = Dot2(ap, ab) / abLenSq;
    t = ClampFloat(t, 0.0f, 1.0f);

    const FVector2D closest = segA + ab * t;
    return (mouse - closest).Length();
}

float GizmoMath::DistancePointToPolyline2D(const FVector2D& mouse,
    const TArray<FVector2D>& points)
{
    const size_t count = points.size();
    if (count == 0) return (std::numeric_limits<float>::max)();
    if (count == 1) return (mouse - points[0]).Length();

    float minDist = (std::numeric_limits<float>::max)();

    for (size_t i = 0; i < count; ++i)
    {
        const size_t next = (i + 1) % count;
        const float d = DistancePointToSegment2D(mouse, points[i], points[next]);
        minDist = (std::min)(minDist, d);
    }

    return minDist;
}

TArray<FVector2D> GizmoMath::SampleRing2D(const FVector& center,
    const FVector& normal,
    float radius,
    int32 samples,
    const FMatrix& viewProj,
    int32 viewportW, int32 viewportH)
{
    TArray<FVector2D> ring;

    if (samples < 3 || radius <= 0.0f)
        return ring;

    const FVector n = normal.Normalized();
    const FVector axis1 = MakeStablePerpendicular(n);
    const FVector axis2 = n.Cross(axis1).Normalized();

    const float step = (2.0f * PI) / static_cast<float>(samples);

    for (int32 i = 0; i < samples; ++i)
    {
        const float theta = step * static_cast<float>(i);
        const FVector p = center
            + axis1 * (std::cos(theta) * radius)
            + axis2 * (std::sin(theta) * radius);

        ring.push_back(WorldToScreen(p, viewProj, viewportW, viewportH));
    }

    return ring;
}

bool GizmoMath::PointInRect2D(const FVector2D& mouse,
    const FVector2D& center,
    float halfSize)
{
    return std::fabs(mouse.x - center.x) <= halfSize
        && std::fabs(mouse.y - center.y) <= halfSize;
}

bool GizmoMath::RayPlaneIntersection(const Ray& ray,
    const FVector& planeOrigin,
    const FVector& planeNormal,
    FVector& outHit)
{
    constexpr float eps = 1e-6f;

    const FVector n = planeNormal.Normalized();
    const float denom = ray.Direction.Dot(n);

    if (std::fabs(denom) < eps)
        return false;

    const float t = (planeOrigin - ray.Origin).Dot(n) / denom;
    if (t < 0.0f)
        return false;

    outHit = ray.Origin + ray.Direction * t;
    return true;
}

bool GizmoMath::ClosestAxisParameterToRay(const FVector& axisOrigin,
    const FVector& axisDir,
    const Ray& ray,
    float& outAxisT)
{
    const FVector u = axisDir.Normalized();
    const FVector v = ray.Direction.Normalized();
    const FVector w0 = axisOrigin - ray.Origin;

    const float a = u.Dot(u);
    const float b = u.Dot(v);
    const float c = v.Dot(v);
    const float d = u.Dot(w0);
    const float e = v.Dot(w0);

    const float denom = a * c - b * b;
    constexpr float eps = 1e-6f;

    if (std::fabs(denom) < eps)
        return false;

    outAxisT = (b * e - c * d) / denom;
    return true;
}

float GizmoMath::SignedAngleAroundAxis(const FVector& fromDir,
    const FVector& toDir,
    const FVector& axis)
{
    const FVector f = fromDir.Normalized();
    const FVector t = toDir.Normalized();
    const FVector n = axis.Normalized();

    const float dot = ClampFloat(f.Dot(t), -1.0f, 1.0f);
    const float sinv = n.Dot(f.Cross(t));
    return std::atan2(sinv, dot);
}

FVector GizmoMath::MakeStablePerpendicular(const FVector& dir)
{
    const FVector n = dir.Normalized();

    const float ax = std::fabs(n.x);
    const float ay = std::fabs(n.y);
    const float az = std::fabs(n.z);

    FVector helper =
        (ax <= ay && ax <= az) ? FVector(1.f, 0.f, 0.f) :
        (ay <= ax && ay <= az) ? FVector(0.f, 1.f, 0.f) :
        FVector(0.f, 0.f, 1.f);

    return helper.Cross(n).Normalized();
}