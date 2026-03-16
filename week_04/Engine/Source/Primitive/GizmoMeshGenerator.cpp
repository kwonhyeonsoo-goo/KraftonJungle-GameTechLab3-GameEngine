#include "GizmoMeshGenerator.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;
const FVector4 kColorWhite(1.0f, 1.0f, 1.0f, 1.0f);
const FVector4 kColorRed(1.0f, 0.0f, 0.0f, 1.0f);
const FVector4 kColorGreen(0.0f, 1.0f, 0.0f, 1.0f);
const FVector4 kColorBlue(0.0f, 0.0f, 1.0f, 1.0f);
const FVector4 kColorYellow(1.0f, 1.0f, 0.0f, 1.0f);
const FVector4 kColorMagenta(1.0f, 0.0f, 1.0f, 1.0f);
const FVector4 kColorCyan(0.0f, 1.0f, 1.0f, 1.0f);

struct Basis3
{
    FVector x;
    FVector y;
    FVector z;
};

FVector NormalizeSafe(const FVector& v, const FVector& fallback)
{
    const FVector normalized = v.GetSafeNormal(1.0e-12f);
    if (normalized.IsNearlyZero())
    {
        return fallback;
    }

    return normalized;
}

Basis3 MakeBasisFromX(const FVector& axis)
{
    const FVector x = NormalizeSafe(axis, FVector::ForwardVector);
    const FVector up = (std::fabs(x.X) < 0.95f) ? FVector::ForwardVector : FVector::RightVector;
    const FVector z = NormalizeSafe(FVector::CrossProduct(x, up), FVector::UpVector);
    const FVector y = NormalizeSafe(FVector::CrossProduct(z, x), FVector::RightVector);
    return Basis3{x, y, z};
}

Basis3 MakeBasisFromNormal(const FVector& normal)
{
    return MakeBasisFromX(normal);
}

FVector TransformPoint(const Basis3& basis, const FVector& origin, const FVector& local)
{
    return origin + basis.x * local.X + basis.y * local.Y + basis.z * local.Z;
}

FVector TransformDirection(const Basis3& basis, const FVector& local)
{
    return basis.x * local.X + basis.y * local.Y + basis.z * local.Z;
}

void AppendTriangle(Mesh& mesh, uint32 a, uint32 b, uint32 c)
{
    mesh.indices.push_back(a);
    mesh.indices.push_back(b);
    mesh.indices.push_back(c);
}

void AppendVertex(Mesh& mesh, const FVector& position, const FVector& normal)
{
    mesh.vertices.push_back(Vertex{position, kColorWhite, normal});
}

void PaintMesh(Mesh& mesh, const FVector4& color)
{
    for (Vertex& vertex : mesh.vertices)
    {
        vertex.color = color;
    }
}

void AppendQuad(
    Mesh& mesh,
    const FVector& v0,
    const FVector& v1,
    const FVector& v2,
    const FVector& v3,
    const FVector& normal)
{
    const uint32 base = static_cast<uint32>(mesh.vertices.size());
    AppendVertex(mesh, v0, normal);
    AppendVertex(mesh, v1, normal);
    AppendVertex(mesh, v2, normal);
    AppendVertex(mesh, v3, normal);
    AppendTriangle(mesh, base + 0, base + 1, base + 2);
    AppendTriangle(mesh, base + 0, base + 2, base + 3);
}

void AppendOrientedBox(Mesh& mesh, const FVector& center, const Basis3& basis, const FVector& halfExtents)
{
    const FVector px = basis.x * halfExtents.X;
    const FVector py = basis.y * halfExtents.Y;
    const FVector pz = basis.z * halfExtents.Z;

    const FVector corners[8] = {
        center - px - py - pz,
        center + px - py - pz,
        center + px + py - pz,
        center - px + py - pz,
        center - px - py + pz,
        center + px - py + pz,
        center + px + py + pz,
        center - px + py + pz};

    AppendQuad(mesh, corners[1], corners[5], corners[6], corners[2], basis.x);
    AppendQuad(mesh, corners[4], corners[0], corners[3], corners[7], basis.x * -1.0f);
    AppendQuad(mesh, corners[3], corners[2], corners[6], corners[7], basis.y);
    AppendQuad(mesh, corners[0], corners[4], corners[5], corners[1], basis.y * -1.0f);
    AppendQuad(mesh, corners[4], corners[5], corners[6], corners[7], basis.z);
    AppendQuad(mesh, corners[0], corners[3], corners[2], corners[1], basis.z * -1.0f);
}

void AppendCylinder(
    Mesh& mesh,
    const FVector& start,
    const FVector& end,
    float radius,
    uint32 sides)
{
    if (sides < 3)
    {
        sides = 3;
    }

    const FVector axis = NormalizeSafe(end - start, FVector::ForwardVector);
    const Basis3 basis = MakeBasisFromX(axis);

    const uint32 sideBase = static_cast<uint32>(mesh.vertices.size());
    for (uint32 i = 0; i < sides; ++i)
    {
        const float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(sides);
        const FVector radial = basis.y * std::cos(angle) + basis.z * std::sin(angle);
        AppendVertex(mesh, start + radial * radius, radial);
        AppendVertex(mesh, end + radial * radius, radial);
    }

    for (uint32 i = 0; i < sides; ++i)
    {
        const uint32 next = (i + 1) % sides;
        const uint32 v0 = sideBase + i * 2 + 0;
        const uint32 v1 = sideBase + i * 2 + 1;
        const uint32 v2 = sideBase + next * 2 + 1;
        const uint32 v3 = sideBase + next * 2 + 0;

        AppendTriangle(mesh, v0, v2, v1);
        AppendTriangle(mesh, v0, v3, v2);
    }

    const uint32 startCenterIndex = static_cast<uint32>(mesh.vertices.size());
    AppendVertex(mesh, start, axis * -1.0f);
    for (uint32 i = 0; i < sides; ++i)
    {
        const float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(sides);
        const FVector radial = basis.y * std::cos(angle) + basis.z * std::sin(angle);
        AppendVertex(mesh, start + radial * radius, axis * -1.0f);
    }

    for (uint32 i = 0; i < sides; ++i)
    {
        const uint32 current = startCenterIndex + 1 + i;
        const uint32 next = startCenterIndex + 1 + ((i + 1) % sides);
        AppendTriangle(mesh, startCenterIndex, next, current);
    }

    const uint32 endCenterIndex = static_cast<uint32>(mesh.vertices.size());
    AppendVertex(mesh, end, axis);
    for (uint32 i = 0; i < sides; ++i)
    {
        const float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(sides);
        const FVector radial = basis.y * std::cos(angle) + basis.z * std::sin(angle);
        AppendVertex(mesh, end + radial * radius, axis);
    }

    for (uint32 i = 0; i < sides; ++i)
    {
        const uint32 current = endCenterIndex + 1 + i;
        const uint32 next = endCenterIndex + 1 + ((i + 1) % sides);
        AppendTriangle(mesh, endCenterIndex, current, next);
    }
}

void AppendCone(
    Mesh& mesh,
    const FVector& baseCenter,
    const FVector& tip,
    float baseRadius,
    uint32 sides,
    bool cap)
{
    if (sides < 3)
    {
        sides = 3;
    }

    const FVector axis = NormalizeSafe(tip - baseCenter, FVector::ForwardVector);
    const Basis3 basis = MakeBasisFromX(axis);

    for (uint32 i = 0; i < sides; ++i)
    {
        const uint32 next = (i + 1) % sides;
        const float angle0 = kTwoPi * static_cast<float>(i) / static_cast<float>(sides);
        const float angle1 = kTwoPi * static_cast<float>(next) / static_cast<float>(sides);

        const FVector radial0 = basis.y * std::cos(angle0) + basis.z * std::sin(angle0);
        const FVector radial1 = basis.y * std::cos(angle1) + basis.z * std::sin(angle1);
        const FVector p0 = baseCenter + radial0 * baseRadius;
        const FVector p1 = baseCenter + radial1 * baseRadius;

        const FVector faceNormal = NormalizeSafe(FVector::CrossProduct(p0 - tip, p1 - tip), radial0);
        const uint32 base = static_cast<uint32>(mesh.vertices.size());

        AppendVertex(mesh, tip, faceNormal);
        AppendVertex(mesh, p0, faceNormal);
        AppendVertex(mesh, p1, faceNormal);
        AppendTriangle(mesh, base + 0, base + 1, base + 2);
    }

    if (!cap)
    {
        return;
    }

    const uint32 centerIndex = static_cast<uint32>(mesh.vertices.size());
    AppendVertex(mesh, baseCenter, axis * -1.0f);
    for (uint32 i = 0; i < sides; ++i)
    {
        const float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(sides);
        const FVector radial = basis.y * std::cos(angle) + basis.z * std::sin(angle);
        AppendVertex(mesh, baseCenter + radial * baseRadius, axis * -1.0f);
    }

    for (uint32 i = 0; i < sides; ++i)
    {
        const uint32 current = centerIndex + 1 + i;
        const uint32 next = centerIndex + 1 + ((i + 1) % sides);
        AppendTriangle(mesh, centerIndex, next, current);
    }
}

void AppendSphere(Mesh& mesh, const FVector& center, float radius, uint32 slices, uint32 stacks)
{
    slices = std::max<uint32>(slices, 3);
    stacks = std::max<uint32>(stacks, 2);

    const uint32 base = static_cast<uint32>(mesh.vertices.size());
    for (uint32 stack = 0; stack <= stacks; ++stack)
    {
        const float v = static_cast<float>(stack) / static_cast<float>(stacks);
        const float phi = v * kPi;
        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);

        for (uint32 slice = 0; slice <= slices; ++slice)
        {
            const float u = static_cast<float>(slice) / static_cast<float>(slices);
            const float theta = u * kTwoPi;
            const float sinTheta = std::sin(theta);
            const float cosTheta = std::cos(theta);

            const FVector normal(
                sinPhi * cosTheta,
                sinPhi * sinTheta,
                cosPhi);

            AppendVertex(mesh, center + normal * radius, normal);
        }
    }

    const uint32 ringVertexCount = slices + 1;
    for (uint32 stack = 0; stack < stacks; ++stack)
    {
        for (uint32 slice = 0; slice < slices; ++slice)
        {
            const uint32 a = base + stack * ringVertexCount + slice;
            const uint32 b = a + ringVertexCount;
            const uint32 c = b + 1;
            const uint32 d = a + 1;

            AppendTriangle(mesh, a, b, c);
            AppendTriangle(mesh, a, c, d);
        }
    }
}

void AppendTorus(
    Mesh& mesh,
    const FVector& center,
    const FVector& normal,
    float majorRadius,
    float minorRadius,
    uint32 majorSegments,
    uint32 minorSegments)
{
    majorSegments = std::max<uint32>(majorSegments, 3);
    minorSegments = std::max<uint32>(minorSegments, 3);

    const Basis3 basis = MakeBasisFromNormal(normal);
    const uint32 base = static_cast<uint32>(mesh.vertices.size());

    for (uint32 major = 0; major <= majorSegments; ++major)
    {
        const float u = static_cast<float>(major) / static_cast<float>(majorSegments);
        const float outerAngle = u * kTwoPi;
        const float outerCos = std::cos(outerAngle);
        const float outerSin = std::sin(outerAngle);

        const FVector radial = basis.y * outerCos + basis.z * outerSin;
        const FVector ringCenter = center + radial * majorRadius;

        for (uint32 minor = 0; minor <= minorSegments; ++minor)
        {
            const float v = static_cast<float>(minor) / static_cast<float>(minorSegments);
            const float innerAngle = v * kTwoPi;
            const float innerCos = std::cos(innerAngle);
            const float innerSin = std::sin(innerAngle);

            const FVector torusNormal = NormalizeSafe(radial * innerCos + basis.x * innerSin, radial);
            const FVector position = ringCenter + torusNormal * minorRadius;
            AppendVertex(mesh, position, torusNormal);
        }
    }

    const uint32 stride = minorSegments + 1;
    for (uint32 major = 0; major < majorSegments; ++major)
    {
        for (uint32 minor = 0; minor < minorSegments; ++minor)
        {
            const uint32 a = base + major * stride + minor;
            const uint32 b = a + stride;
            const uint32 c = b + 1;
            const uint32 d = a + 1;

            AppendTriangle(mesh, a, b, c);
            AppendTriangle(mesh, a, c, d);
        }
    }
}

void MakeTranslationAxis(Mesh& mesh, const FVector& axis, const TranslationGizmoDesc& desc)
{
    const FVector direction = NormalizeSafe(axis, FVector::ForwardVector);
    const FVector shaftEnd = direction * desc.shaftLength;
    const FVector tip = direction * (desc.shaftLength + desc.arrowLength);
    AppendCylinder(mesh, FVector::ZeroVector, shaftEnd, desc.shaftRadius, desc.axisSides);
    AppendCone(mesh, shaftEnd, tip, desc.arrowRadius, desc.axisSides, true);
}

void MakeScaleAxis(Mesh& mesh, const FVector& axis, const ScaleGizmoDesc& desc)
{
    const FVector direction = NormalizeSafe(axis, FVector::ForwardVector);
    const FVector shaftEnd = direction * desc.shaftLength;
    AppendCylinder(mesh, FVector::ZeroVector, shaftEnd, desc.shaftRadius, desc.axisSides);

    const Basis3 basis = MakeBasisFromX(direction);
    const FVector center = direction * (desc.shaftLength + desc.cubeSize * 0.5f);
    const FVector half(desc.cubeSize * 0.5f, desc.cubeSize * 0.5f, desc.cubeSize * 0.5f);
    AppendOrientedBox(mesh, center, basis, half);
}

void MakeTranslationPlane(Mesh& mesh, const Basis3& basis, float offsetA, float size, float thickness)
{
    const FVector center = basis.x * (offsetA + size * 0.5f) + basis.y * (offsetA + size * 0.5f);
    const FVector half(size * 0.5f, size * 0.5f, thickness * 0.5f);
    const Basis3 planeBasis{basis.x, basis.y, basis.z};
    AppendOrientedBox(mesh, center, planeBasis, half);
}

void MakeScalePlaneBracket(Mesh& mesh, const Basis3& basis, const ScaleGizmoDesc& desc)
{
    const float armHalfLength = desc.planeArmLength * 0.5f;
    const float armHalfWidth = desc.planeArmWidth * 0.5f;
    const float thicknessHalf = desc.planeThickness * 0.5f;

    const FVector firstCenter =
        basis.x * (desc.planeOffset + armHalfLength) + basis.y * desc.planeOffset;
    const FVector secondCenter =
        basis.x * desc.planeOffset + basis.y * (desc.planeOffset + armHalfLength);

    AppendOrientedBox(
        mesh,
        firstCenter,
        Basis3{basis.x, basis.y, basis.z},
        FVector(armHalfLength, armHalfWidth, thicknessHalf));
    AppendOrientedBox(
        mesh,
        secondCenter,
        Basis3{basis.x, basis.y, basis.z},
        FVector(armHalfWidth, armHalfLength, thicknessHalf));
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

void AppendMesh(Mesh& destination, const Mesh& source)
{
    if (source.vertices.empty())
    {
        return;
    }

    const uint32 vertexOffset = static_cast<uint32>(destination.vertices.size());
    destination.vertices.insert(destination.vertices.end(), source.vertices.begin(), source.vertices.end());
    destination.indices.reserve(destination.indices.size() + source.indices.size());
    for (const uint32 index : source.indices)
    {
        destination.indices.push_back(vertexOffset + index);
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

TranslationGizmoMesh GenerateTranslationGizmo(const TranslationGizmoDesc& desc)
{
    TranslationGizmoMesh gizmo;

    MakeTranslationAxis(gizmo.axisX, FVector::ForwardVector, desc);
    MakeTranslationAxis(gizmo.axisY, FVector::RightVector, desc);
    MakeTranslationAxis(gizmo.axisZ, FVector::UpVector, desc);
    PaintMesh(gizmo.axisX, kColorRed);
    PaintMesh(gizmo.axisY, kColorGreen);
    PaintMesh(gizmo.axisZ, kColorBlue);

    MakeTranslationPlane(gizmo.planeXY, Basis3{FVector::ForwardVector, FVector::RightVector, FVector::UpVector}, desc.planeOffset, desc.planeSize, desc.planeThickness);
    MakeTranslationPlane(gizmo.planeXZ, Basis3{FVector::ForwardVector, FVector::UpVector, FVector::LeftVector}, desc.planeOffset, desc.planeSize, desc.planeThickness);
    MakeTranslationPlane(gizmo.planeYZ, Basis3{FVector::RightVector, FVector::UpVector, FVector::ForwardVector}, desc.planeOffset, desc.planeSize, desc.planeThickness);
    PaintMesh(gizmo.planeXY, kColorYellow);
    PaintMesh(gizmo.planeXZ, kColorMagenta);
    PaintMesh(gizmo.planeYZ, kColorCyan);

    if (desc.includeCenterSphere)
    {
        AppendSphere(gizmo.center, FVector::ZeroVector, desc.centerSphereRadius, desc.sphereSlices, desc.sphereStacks);
        PaintMesh(gizmo.center, kColorWhite);
    }

    return gizmo;
}

RotationGizmoMesh GenerateRotationGizmo(const RotationGizmoDesc& desc)
{
    RotationGizmoMesh gizmo;

    AppendTorus(gizmo.ringX, FVector::ZeroVector, FVector::ForwardVector, desc.ringRadius, desc.ringThickness, desc.ringSegments, desc.tubeSegments);
    AppendTorus(gizmo.ringY, FVector::ZeroVector, FVector::RightVector, desc.ringRadius, desc.ringThickness, desc.ringSegments, desc.tubeSegments);
    AppendTorus(gizmo.ringZ, FVector::ZeroVector, FVector::UpVector, desc.ringRadius, desc.ringThickness, desc.ringSegments, desc.tubeSegments);
    PaintMesh(gizmo.ringX, kColorRed);
    PaintMesh(gizmo.ringY, kColorGreen);
    PaintMesh(gizmo.ringZ, kColorBlue);

    if (desc.includeScreenRing)
    {
        AppendTorus(
            gizmo.screenRing,
            FVector::ZeroVector,
            desc.screenNormal,
            desc.screenRingRadius,
            desc.screenRingThickness,
            desc.ringSegments,
            desc.tubeSegments);
        PaintMesh(gizmo.screenRing, kColorWhite);
    }

    if (desc.includeArcballSphere)
    {
        AppendSphere(gizmo.arcball, FVector::ZeroVector, desc.arcballRadius, desc.sphereSlices, desc.sphereStacks);
        PaintMesh(gizmo.arcball, kColorWhite);
    }

    return gizmo;
}

ScaleGizmoMesh GenerateScaleGizmo(const ScaleGizmoDesc& desc)
{
    ScaleGizmoMesh gizmo;

    MakeScaleAxis(gizmo.axisX, FVector::ForwardVector, desc);
    MakeScaleAxis(gizmo.axisY, FVector::RightVector, desc);
    MakeScaleAxis(gizmo.axisZ, FVector::UpVector, desc);
    PaintMesh(gizmo.axisX, kColorRed);
    PaintMesh(gizmo.axisY, kColorGreen);
    PaintMesh(gizmo.axisZ, kColorBlue);

    MakeScalePlaneBracket(gizmo.planeXY, Basis3{FVector::ForwardVector, FVector::RightVector, FVector::UpVector}, desc);
    MakeScalePlaneBracket(gizmo.planeXZ, Basis3{FVector::ForwardVector, FVector::UpVector, FVector::LeftVector}, desc);
    MakeScalePlaneBracket(gizmo.planeYZ, Basis3{FVector::RightVector, FVector::UpVector, FVector::ForwardVector}, desc);
    PaintMesh(gizmo.planeXY, kColorYellow);
    PaintMesh(gizmo.planeXZ, kColorMagenta);
    PaintMesh(gizmo.planeYZ, kColorCyan);

    if (desc.includeCenterCube)
    {
        const float half = desc.centerCubeSize * 0.5f;
        AppendOrientedBox(
            gizmo.center,
            FVector::ZeroVector,
            Basis3{FVector::ForwardVector, FVector::RightVector, FVector::UpVector},
            FVector(half, half, half));
        PaintMesh(gizmo.center, kColorWhite);
    }

    return gizmo;
}

Mesh Combine(const TranslationGizmoMesh& gizmo)
{
    return MergeMeshes(
        {&gizmo.axisX, &gizmo.axisY, &gizmo.axisZ, &gizmo.planeXY, &gizmo.planeXZ, &gizmo.planeYZ, &gizmo.center});
}

Mesh Combine(const RotationGizmoMesh& gizmo)
{
    return MergeMeshes({&gizmo.ringX, &gizmo.ringY, &gizmo.ringZ, &gizmo.screenRing, &gizmo.arcball});
}

Mesh Combine(const ScaleGizmoMesh& gizmo)
{
    return MergeMeshes(
        {&gizmo.axisX, &gizmo.axisY, &gizmo.axisZ, &gizmo.planeXY, &gizmo.planeXZ, &gizmo.planeYZ, &gizmo.center});
}
