#include "UnrealEditorStyledGizmo.h"

#include <algorithm>
#include <cmath>


namespace
{

constexpr float kPi = 3.14159265358979323846f;
constexpr float kHalfPi = 0.5f * kPi;
constexpr float kTwoPi = 2.0f * kPi;

constexpr float kAxisLength = 35.0f;
constexpr float kAxisLengthScaleOffset = 5.0f;
constexpr float kCylinderRadius = 1.2f;
constexpr float kConeHeadOffset = 12.0f;
constexpr float kCubeHeadOffset = 3.0f;
constexpr float kConeScale = -13.0f;
constexpr int kAxisCircleSides = 24;
constexpr float kInnerAxisCircleRadius = 48.0f;
constexpr float kOuterAxisCircleRadius = 56.0f;
constexpr float kTranslateCornerStart = 7.0f;
constexpr float kTranslateCornerLength = 12.0f;
constexpr float kTranslateCornerThickness = 1.2f;
constexpr float kTranslateScreenSphereRadius = 4.0f;
constexpr float kScaleBracketOuter = 24.0f;
constexpr float kScaleBracketInner = 12.0f;
constexpr float kScaleBracketThickness = 1.0f;
constexpr float kScaleCenterCubeHalf = 4.0f;
constexpr float kLargeOuterAlpha = 127.0f / 255.0f;
constexpr float kArcballAlpha = 6.0f / 255.0f;

struct Basis3
{
    FVector x;
    FVector y;
    FVector z;
};

FVector NormalizeSafe(const FVector& v, const FVector& fallback)
{
    const float lenSq = v.SizeSquared();
    if (lenSq <= 1.0e-12f)
    {
        return fallback;
    }

    return v.GetSafeNormal();
}

Basis3 MakeBasisFromX(const FVector& axis)
{
    const FVector x = NormalizeSafe(axis, FVector(1.0f, 0.0f, 0.0f));
    const FVector up = (std::fabs(x.X) < 0.95f) ? FVector(1.0f, 0.0f, 0.0f) : FVector(0.0f, 1.0f, 0.0f);
    const FVector z = NormalizeSafe(FVector::CrossProduct(x, up), FVector(0.0f, 0.0f, 1.0f));
    const FVector y = NormalizeSafe(FVector::CrossProduct(z, x), FVector(0.0f, 1.0f, 0.0f));
    return Basis3{x, y, z};
}

FVector TransformPoint(const Basis3& basis, const FVector& local)
{
    return basis.x * local.X + basis.y * local.Y + basis.z * local.Z;
}

Color MakeColor(float r, float g, float b, float a = 1.0f)
{
    return Color{r, g, b, a};
}

void AppendTriangle(Mesh& mesh, std::uint32_t a, std::uint32_t b, std::uint32_t c)
{
    mesh.indices.push_back(a);
    mesh.indices.push_back(b);
    mesh.indices.push_back(c);
}

void AppendVertex(Mesh& mesh, const FVector& position, const FVector& normal, const Vec2& uv, const Color& color)
{
    mesh.vertices.push_back(Vertex{position, normal, uv, color});
}

void AppendQuad(Mesh& mesh, const FVector& v0, const FVector& v1, const FVector& v2, const FVector& v3, const FVector& normal, const Color& color)
{
    const std::uint32_t base = static_cast<std::uint32_t>(mesh.vertices.size());
    AppendVertex(mesh, v0, normal, Vec2{0.0f, 0.0f}, color);
    AppendVertex(mesh, v1, normal, Vec2{1.0f, 0.0f}, color);
    AppendVertex(mesh, v2, normal, Vec2{1.0f, 1.0f}, color);
    AppendVertex(mesh, v3, normal, Vec2{0.0f, 1.0f}, color);
    AppendTriangle(mesh, base + 0, base + 1, base + 2);
    AppendTriangle(mesh, base + 0, base + 2, base + 3);
}

void AppendOrientedBox(Mesh& mesh, const FVector& center, const Basis3& basis, const FVector& halfExtents, const Color& color)
{
    const FVector px = basis.x * halfExtents.X;
    const FVector py = basis.y * halfExtents.Y;
    const FVector pz = basis.z * halfExtents.Z;

    const FVector c[8] = {
        center - px - py - pz,
        center + px - py - pz,
        center + px + py - pz,
        center - px + py - pz,
        center - px - py + pz,
        center + px - py + pz,
        center + px + py + pz,
        center - px + py + pz};

    AppendQuad(mesh, c[1], c[5], c[6], c[2], basis.x, color);
    AppendQuad(mesh, c[4], c[0], c[3], c[7], basis.x * -1.0f, color);
    AppendQuad(mesh, c[3], c[2], c[6], c[7], basis.y, color);
    AppendQuad(mesh, c[0], c[4], c[5], c[1], basis.y * -1.0f, color);
    AppendQuad(mesh, c[4], c[5], c[6], c[7], basis.z, color);
    AppendQuad(mesh, c[0], c[3], c[2], c[1], basis.z * -1.0f, color);
}

void AppendCylinder(Mesh& mesh, const FVector& start, const FVector& end, float radius, std::uint32_t sides, const Color& color)
{
    sides = std::max<std::uint32_t>(sides, 3);
    const FVector axis = NormalizeSafe(end - start, FVector(1.0f, 0.0f, 0.0f));
    const Basis3 basis = MakeBasisFromX(axis);

    const std::uint32_t base = static_cast<std::uint32_t>(mesh.vertices.size());
    for (std::uint32_t i = 0; i < sides; ++i)
    {
        const float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(sides);
        const FVector radial = basis.y * std::cos(angle) + basis.z * std::sin(angle);
        AppendVertex(mesh, start + radial * radius, radial, Vec2{0.0f, 0.0f}, color);
        AppendVertex(mesh, end + radial * radius, radial, Vec2{1.0f, 0.0f}, color);
    }

    for (std::uint32_t i = 0; i < sides; ++i)
    {
        const std::uint32_t next = (i + 1) % sides;
        const std::uint32_t v0 = base + i * 2 + 0;
        const std::uint32_t v1 = base + i * 2 + 1;
        const std::uint32_t v2 = base + next * 2 + 1;
        const std::uint32_t v3 = base + next * 2 + 0;
        AppendTriangle(mesh, v0, v2, v1);
        AppendTriangle(mesh, v0, v3, v2);
    }
}

FVector CalcConeVert(float angle1, float angle2, float azimuthAngle)
{
    const float sinX_2 = std::sin(0.5f * angle1);
    const float sinY_2 = std::sin(0.5f * angle2);
    const float sinSqX_2 = sinX_2 * sinX_2;
    const float sinSqY_2 = sinY_2 * sinY_2;
    const float phi = std::atan2(std::sin(azimuthAngle) * sinSqY_2, std::cos(azimuthAngle) * sinSqX_2);
    const float sinPhi = std::sin(phi);
    const float cosPhi = std::cos(phi);
    const float sinSqPhi = sinPhi * sinPhi;
    const float cosSqPhi = cosPhi * cosPhi;
    const float rSq = sinSqX_2 * sinSqY_2 / (sinSqX_2 * sinSqPhi + sinSqY_2 * cosSqPhi);
    const float r = std::sqrt(rSq);
    const float sqr = std::sqrt(1.0f - rSq);
    const float alpha = r * cosPhi;
    const float beta = r * sinPhi;
    return FVector(1.0f - 2.0f * rSq, 2.0f * sqr * alpha, 2.0f * sqr * beta);
}

void AppendUnrealCone(Mesh& mesh, const FVector& tipPosition, const FVector& axisDirection, float scale, float angle, std::uint32_t sides, const Color& color)
{
    const Basis3 basis = MakeBasisFromX(axisDirection);
    for (std::uint32_t i = 0; i < sides; ++i)
    {
        const std::uint32_t next = (i + 1) % sides;
        const float a0 = kTwoPi * static_cast<float>(i) / static_cast<float>(sides);
        const float a1 = kTwoPi * static_cast<float>(next) / static_cast<float>(sides);
        const FVector tip = tipPosition;
        const FVector p0 = tipPosition + TransformPoint(basis, CalcConeVert(angle, angle, a0) * scale);
        const FVector p1 = tipPosition + TransformPoint(basis, CalcConeVert(angle, angle, a1) * scale);
        const FVector n = NormalizeSafe(FVector::CrossProduct(p0 - tip, p1 - tip), axisDirection);
        const std::uint32_t base = static_cast<std::uint32_t>(mesh.vertices.size());
        AppendVertex(mesh, tip, n, Vec2{0.0f, 0.5f}, color);
        AppendVertex(mesh, p0, n, Vec2{1.0f, 0.0f}, color);
        AppendVertex(mesh, p1, n, Vec2{1.0f, 1.0f}, color);
        AppendTriangle(mesh, base + 0, base + 1, base + 2);
    }
}

void AppendSphere(Mesh& mesh, const FVector& center, float radius, std::uint32_t slices, std::uint32_t stacks, const Color& color)
{
    slices = std::max<std::uint32_t>(slices, 3);
    stacks = std::max<std::uint32_t>(stacks, 2);
    const std::uint32_t base = static_cast<std::uint32_t>(mesh.vertices.size());

    for (std::uint32_t stack = 0; stack <= stacks; ++stack)
    {
        const float v = static_cast<float>(stack) / static_cast<float>(stacks);
        const float phi = v * kPi;
        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);

        for (std::uint32_t slice = 0; slice <= slices; ++slice)
        {
            const float u = static_cast<float>(slice) / static_cast<float>(slices);
            const float theta = u * kTwoPi;
            const FVector n(sinPhi * std::cos(theta), sinPhi * std::sin(theta), cosPhi);
            AppendVertex(mesh, center + n * radius, n, Vec2{u, v}, color);
        }
    }

    const std::uint32_t stride = slices + 1;
    for (std::uint32_t stack = 0; stack < stacks; ++stack)
    {
        for (std::uint32_t slice = 0; slice < slices; ++slice)
        {
            const std::uint32_t a = base + stack * stride + slice;
            const std::uint32_t b = a + stride;
            const std::uint32_t c = b + 1;
            const std::uint32_t d = a + 1;
            AppendTriangle(mesh, a, b, c);
            AppendTriangle(mesh, a, c, d);
        }
    }
}

void AppendArcBand(Mesh& mesh, const FVector& axis0, const FVector& axis1, float innerRadius, float outerRadius, float startAngle, float endAngle, const Color& color)
{
    const float range = endAngle - startAngle;
    const std::uint32_t points = std::max<std::uint32_t>(2, static_cast<std::uint32_t>(std::floor(kAxisCircleSides * range / kHalfPi)) + 1);
    const FVector normal = NormalizeSafe(FVector::CrossProduct(axis0, axis1), FVector(0.0f, 0.0f, 1.0f));

    const std::uint32_t outerBase = static_cast<std::uint32_t>(mesh.vertices.size());
    for (std::uint32_t i = 0; i <= points; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(points);
        const float angle = startAngle + range * t;
        const FVector dir = NormalizeSafe(axis0 * std::cos(angle) + axis1 * std::sin(angle), axis0);
        AppendVertex(mesh, dir * outerRadius, normal, Vec2{t, 0.0f}, color);
    }

    const std::uint32_t innerBase = static_cast<std::uint32_t>(mesh.vertices.size());
    for (std::uint32_t i = 0; i <= points; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(points);
        const float angle = startAngle + range * t;
        const FVector dir = NormalizeSafe(axis0 * std::cos(angle) + axis1 * std::sin(angle), axis0);
        AppendVertex(mesh, dir * innerRadius, normal, Vec2{t, 1.0f}, color);
    }

    for (std::uint32_t i = 0; i < points; ++i)
    {
        AppendTriangle(mesh, outerBase + i, outerBase + i + 1, innerBase + i);
        AppendTriangle(mesh, outerBase + i + 1, innerBase + i + 1, innerBase + i);
    }
}

void AppendTranslatePlane(Mesh& mesh, const Basis3& basis, const Color& axis0Color, const Color& axis1Color, float s)
{
    const float start = kTranslateCornerStart * s;
    const float length = kTranslateCornerLength * s;
    const float thick = kTranslateCornerThickness * s * 0.5f;
    const FVector xCenter = basis.x * (start + length * 0.5f) + basis.y * start;
    const FVector yCenter = basis.x * start + basis.y * (start + length * 0.5f);
    AppendOrientedBox(mesh, xCenter, Basis3{basis.x, basis.y, basis.z}, FVector(length * 0.5f, thick, thick), axis0Color);
    AppendOrientedBox(mesh, yCenter, Basis3{basis.x, basis.y, basis.z}, FVector(thick, length * 0.5f, thick), axis1Color);
}

void AppendScalePlane(Mesh& mesh, const Basis3& basis, const Color& axis0Color, const Color& axis1Color, float s)
{
    const float outer = kScaleBracketOuter * s;
    const float inner = kScaleBracketInner * s;
    const float halfT = kScaleBracketThickness * s * 0.5f;
    const float diagLength = std::sqrt((outer - inner) * (outer - inner) * 2.0f) * 0.5f;
    const FVector diag = NormalizeSafe(basis.x + basis.y, basis.x);
    const FVector side = NormalizeSafe(FVector::CrossProduct(basis.z, diag), basis.y);
    const FVector c0 = basis.x * ((outer + inner) * 0.5f) + basis.y * inner;
    const FVector c1 = basis.x * inner + basis.y * ((outer + inner) * 0.5f);
    AppendOrientedBox(mesh, c0, Basis3{diag, side, basis.z}, FVector(diagLength, halfT, halfT), axis0Color);
    AppendOrientedBox(mesh, c1, Basis3{diag, side, basis.z}, FVector(diagLength, halfT, halfT), axis1Color);
}

} // namespace

void Mesh::Clear()
{
    vertices.clear();
    indices.clear();
}

bool Mesh::Empty() const
{
    return vertices.empty() || indices.empty();
}

Color AxisColorX()
{
    return MakeColor(0.594f, 0.0197f, 0.0f, 1.0f);
}

Color AxisColorY()
{
    return MakeColor(0.1349f, 0.3959f, 0.0f, 1.0f);
}

Color AxisColorZ()
{
    return MakeColor(0.0251f, 0.207f, 0.85f, 1.0f);
}

Color ScreenAxisColor()
{
    return MakeColor(0.76f, 0.72f, 0.14f, 1.0f);
}

Color ScreenSpaceColor()
{
    return MakeColor(196.0f / 255.0f, 196.0f / 255.0f, 196.0f / 255.0f, 1.0f);
}

Color ArcballColor()
{
    return MakeColor(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, kArcballAlpha);
}

Color HighlightColor()
{
    return MakeColor(1.0f, 1.0f, 0.0f, 1.0f);
}

void AppendMesh(Mesh& destination, const Mesh& source)
{
    if (source.vertices.empty())
    {
        return;
    }

    const std::uint32_t offset = static_cast<std::uint32_t>(destination.vertices.size());
    destination.vertices.insert(destination.vertices.end(), source.vertices.begin(), source.vertices.end());
    for (std::uint32_t index : source.indices)
    {
        destination.indices.push_back(offset + index);
    }
}

Mesh MergeMeshes(std::initializer_list<const Mesh*> meshes)
{
    Mesh merged;
    for (const Mesh* mesh : meshes)
    {
        if (mesh != nullptr)
        {
            AppendMesh(merged, *mesh);
        }
    }
    return merged;
}

TranslationGizmo GenerateTranslationGizmo(const TranslationDesc& desc)
{
    TranslationGizmo gizmo;
    const float s = desc.uniformScale;
    const float axisLength = kAxisLength * s;
    const float coneTip = (kAxisLength + kConeHeadOffset) * s;
    const float coneAngle = (kPi * 5.0f) * (kPi / 180.0f);

    AppendCylinder(gizmo.axisX, FVector(0.0f, 0.0f, 0.0f), FVector(axisLength, 0.0f, 0.0f), kCylinderRadius * s, 16, AxisColorX());
    AppendCylinder(gizmo.axisY, FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, axisLength, 0.0f), kCylinderRadius * s, 16, AxisColorY());
    AppendCylinder(gizmo.axisZ, FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, axisLength), kCylinderRadius * s, 16, AxisColorZ());

    AppendUnrealCone(gizmo.axisX, FVector(coneTip, 0.0f, 0.0f), FVector(1.0f, 0.0f, 0.0f), kConeScale * s, coneAngle, 32, AxisColorX());
    AppendUnrealCone(gizmo.axisY, FVector(0.0f, coneTip, 0.0f), FVector(0.0f, 1.0f, 0.0f), kConeScale * s, coneAngle, 32, AxisColorY());
    AppendUnrealCone(gizmo.axisZ, FVector(0.0f, 0.0f, coneTip), FVector(0.0f, 0.0f, 1.0f), kConeScale * s, coneAngle, 32, AxisColorZ());

    AppendTranslatePlane(gizmo.planeXY, Basis3{FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f)}, AxisColorX(), AxisColorY(), s);
    AppendTranslatePlane(gizmo.planeXZ, Basis3{FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f), FVector(0.0f, -1.0f, 0.0f)}, AxisColorX(), AxisColorZ(), s);
    AppendTranslatePlane(gizmo.planeYZ, Basis3{FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f), FVector(1.0f, 0.0f, 0.0f)}, AxisColorY(), AxisColorZ(), s);

    if (desc.includeScreenHandle)
    {
        AppendSphere(gizmo.screenSphere, FVector(0.0f, 0.0f, 0.0f), kTranslateScreenSphereRadius * s, 10, 5, ScreenSpaceColor());
    }

    return gizmo;
}

RotationGizmo GenerateRotationGizmo(const RotationDesc& desc)
{
    RotationGizmo gizmo;
    const float s = desc.uniformScale;
    const float innerRadius = kInnerAxisCircleRadius * s;
    const float outerRadius = kOuterAxisCircleRadius * s;
    const FVector dir = NormalizeSafe(desc.cameraDirection, FVector(-1.0f, -1.0f, -1.0f));

    auto addAxisArc = [&](Mesh& mesh, const FVector& axis0, const FVector& axis1, const Color& baseColor)
    {
        const Color arcColor = MakeColor(baseColor.r, baseColor.g, baseColor.b, kLargeOuterAlpha);
        if (desc.fullAxisRings)
        {
            AppendArcBand(mesh, axis0, axis1, innerRadius, outerRadius, 0.0f, kTwoPi, arcColor);
            return;
        }

        const bool mirror0 = FVector::DotProduct(axis0, dir) <= 0.0f;
        const bool mirror1 = FVector::DotProduct(axis1, dir) <= 0.0f;
        const FVector render0 = mirror0 ? axis0 : axis0 * -1.0f;
        const FVector render1 = mirror1 ? axis1 : axis1 * -1.0f;
        AppendArcBand(mesh, render0, render1, innerRadius, outerRadius, 0.0f, kHalfPi, arcColor);
    };

    addAxisArc(gizmo.ringX, FVector(0.0f, 0.0f, 1.0f), FVector(0.0f, 1.0f, 0.0f), AxisColorX());
    addAxisArc(gizmo.ringY, FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f), AxisColorY());
    addAxisArc(gizmo.ringZ, FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), AxisColorZ());

    if (desc.includeScreenRing)
    {
        const float screenOuter = kOuterAxisCircleRadius * 1.25f * s;
        const float screenInner = (kOuterAxisCircleRadius - 1.0f) * 1.25f * s;
        AppendArcBand(
            gizmo.screenRing,
            NormalizeSafe(desc.viewUp, FVector(0.0f, 1.0f, 0.0f)),
            NormalizeSafe(desc.viewRight, FVector(1.0f, 0.0f, 0.0f)),
            screenInner,
            screenOuter,
            0.0f,
            kTwoPi,
            ScreenAxisColor());
    }

    if (desc.includeArcball)
    {
        AppendSphere(gizmo.arcball, FVector(0.0f, 0.0f, 0.0f), innerRadius, 32, 24, ArcballColor());
    }

    return gizmo;
}

ScaleGizmo GenerateScaleGizmo(const ScaleDesc& desc)
{
    ScaleGizmo gizmo;
    const float s = desc.uniformScale;
    const float axisLength = (kAxisLength - (kAxisLengthScaleOffset * 2.0f)) * s;
    const float cubeCenter = (kAxisLength + kCubeHeadOffset - kAxisLengthScaleOffset) * s;

    AppendCylinder(gizmo.axisX, FVector(0.0f, 0.0f, 0.0f), FVector(axisLength, 0.0f, 0.0f), kCylinderRadius * s, 16, AxisColorX());
    AppendCylinder(gizmo.axisY, FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, axisLength, 0.0f), kCylinderRadius * s, 16, AxisColorY());
    AppendCylinder(gizmo.axisZ, FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, axisLength), kCylinderRadius * s, 16, AxisColorZ());

    AppendOrientedBox(gizmo.axisX, FVector(cubeCenter, 0.0f, 0.0f), Basis3{FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f)}, FVector(kScaleCenterCubeHalf * s, kScaleCenterCubeHalf * s, kScaleCenterCubeHalf * s), AxisColorX());
    AppendOrientedBox(gizmo.axisY, FVector(0.0f, cubeCenter, 0.0f), Basis3{FVector(0.0f, 1.0f, 0.0f), FVector(-1.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f)}, FVector(kScaleCenterCubeHalf * s, kScaleCenterCubeHalf * s, kScaleCenterCubeHalf * s), AxisColorY());
    AppendOrientedBox(gizmo.axisZ, FVector(0.0f, 0.0f, cubeCenter), Basis3{FVector(0.0f, 0.0f, 1.0f), FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f)}, FVector(kScaleCenterCubeHalf * s, kScaleCenterCubeHalf * s, kScaleCenterCubeHalf * s), AxisColorZ());

    AppendScalePlane(gizmo.planeXY, Basis3{FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f)}, AxisColorX(), AxisColorY(), s);
    AppendScalePlane(gizmo.planeXZ, Basis3{FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f), FVector(0.0f, -1.0f, 0.0f)}, AxisColorX(), AxisColorZ(), s);
    AppendScalePlane(gizmo.planeYZ, Basis3{FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f), FVector(1.0f, 0.0f, 0.0f)}, AxisColorY(), AxisColorZ(), s);

    if (desc.includeCenterCube)
    {
        AppendOrientedBox(gizmo.centerCube, FVector(0.0f, 0.0f, 0.0f), Basis3{FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f)}, FVector(kScaleCenterCubeHalf * s, kScaleCenterCubeHalf * s, kScaleCenterCubeHalf * s), ScreenSpaceColor());
    }

    return gizmo;
}

Mesh Combine(const TranslationGizmo& gizmo)
{
    return MergeMeshes({&gizmo.axisX, &gizmo.axisY, &gizmo.axisZ, &gizmo.planeXY, &gizmo.planeXZ, &gizmo.planeYZ, &gizmo.screenSphere});
}

Mesh Combine(const RotationGizmo& gizmo)
{
    return MergeMeshes({&gizmo.ringX, &gizmo.ringY, &gizmo.ringZ, &gizmo.screenRing, &gizmo.arcball});
}

Mesh Combine(const ScaleGizmo& gizmo)
{
    return MergeMeshes({&gizmo.axisX, &gizmo.axisY, &gizmo.axisZ, &gizmo.planeXY, &gizmo.planeXZ, &gizmo.planeYZ, &gizmo.centerCube});
}
