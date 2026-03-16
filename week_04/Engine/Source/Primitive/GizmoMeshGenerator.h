#pragma once

#include "Math/Vector.h"
#include "Math/Vector4.h"

#include <cstdint>
#include <initializer_list>
#include <vector>

struct Vertex
{
    FVector position;
    FVector4 color;
    FVector normal;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<uint32> indices;

    void Clear();
    bool Empty() const;
};

struct TranslationGizmoDesc
{
    float shaftLength = 1.0f;
    float shaftRadius = 0.035f;
    float arrowLength = 0.22f;
    float arrowRadius = 0.085f;
    uint32 axisSides = 24;

    float planeOffset = 0.22f;
    float planeSize = 0.18f;
    float planeThickness = 0.01f;

    bool includeCenterSphere = false;
    float centerSphereRadius = 0.07f;
    uint32 sphereSlices = 20;
    uint32 sphereStacks = 12;
};

struct TranslationGizmoMesh
{
    Mesh axisX;
    Mesh axisY;
    Mesh axisZ;
    Mesh planeXY;
    Mesh planeXZ;
    Mesh planeYZ;
    Mesh center;
};

struct RotationGizmoDesc
{
    float ringRadius = 1.0f;
    float ringThickness = 0.045f;
    uint32 ringSegments = 64;
    uint32 tubeSegments = 14;

    bool includeScreenRing = true;
    FVector screenNormal = FVector(0.0f, 0.0f, 1.0f);
    float screenRingRadius = 1.18f;
    float screenRingThickness = 0.03f;

    bool includeArcballSphere = false;
    float arcballRadius = 0.82f;
    uint32 sphereSlices = 32;
    uint32 sphereStacks = 18;
};

struct RotationGizmoMesh
{
    Mesh ringX;
    Mesh ringY;
    Mesh ringZ;
    Mesh screenRing;
    Mesh arcball;
};

struct ScaleGizmoDesc
{
    float shaftLength = 1.0f;
    float shaftRadius = 0.03f;
    float cubeSize = 0.13f;
    uint32 axisSides = 20;

    float planeOffset = 0.26f;
    float planeArmLength = 0.20f;
    float planeArmWidth = 0.03f;
    float planeThickness = 0.02f;

    bool includeCenterCube = true;
    float centerCubeSize = 0.1f;
};

struct ScaleGizmoMesh
{
    Mesh axisX;
    Mesh axisY;
    Mesh axisZ;
    Mesh planeXY;
    Mesh planeXZ;
    Mesh planeYZ;
    Mesh center;
};

TranslationGizmoMesh GenerateTranslationGizmo(const TranslationGizmoDesc& desc = {});
RotationGizmoMesh GenerateRotationGizmo(const RotationGizmoDesc& desc = {});
ScaleGizmoMesh GenerateScaleGizmo(const ScaleGizmoDesc& desc = {});

void AppendMesh(Mesh& destination, const Mesh& source);
Mesh MergeMeshes(std::initializer_list<const Mesh*> meshes);

Mesh Combine(const TranslationGizmoMesh& gizmo);
Mesh Combine(const RotationGizmoMesh& gizmo);
Mesh Combine(const ScaleGizmoMesh& gizmo);