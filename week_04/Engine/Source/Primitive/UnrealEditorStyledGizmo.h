#pragma once

#include <cstdint>
#include <initializer_list>
#include <vector>
#include "Math/Vector.h"


struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;
};

struct Color
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

struct Vertex
{
    FVector position;
    FVector normal;
    Vec2 uv;
    Color color;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    void Clear();
    bool Empty() const;
};

struct TranslationDesc
{
    float uniformScale = 1.0f;
    bool includeScreenHandle = true;
};

struct RotationDesc
{
    float uniformScale = 1.0f;
    FVector cameraDirection = FVector(-1.0f, -1.0f, -1.0f);
    FVector viewUp = FVector(0.0f, 1.0f, 0.0f);
    FVector viewRight = FVector(1.0f, 0.0f, 0.0f);
    bool fullAxisRings = false;
    bool includeScreenRing = true;
    bool includeArcball = true;
};

struct ScaleDesc
{
    float uniformScale = 1.0f;
    bool includeCenterCube = true;
};

struct TranslationGizmo
{
    Mesh axisX;
    Mesh axisY;
    Mesh axisZ;
    Mesh planeXY;
    Mesh planeXZ;
    Mesh planeYZ;
    Mesh screenSphere;
};

struct RotationGizmo
{
    Mesh ringX;
    Mesh ringY;
    Mesh ringZ;
    Mesh screenRing;
    Mesh arcball;
};

struct ScaleGizmo
{
    Mesh axisX;
    Mesh axisY;
    Mesh axisZ;
    Mesh planeXY;
    Mesh planeXZ;
    Mesh planeYZ;
    Mesh centerCube;
};

TranslationGizmo GenerateTranslationGizmo(const TranslationDesc& desc = {});
RotationGizmo GenerateRotationGizmo(const RotationDesc& desc = {});
ScaleGizmo GenerateScaleGizmo(const ScaleDesc& desc = {});

void AppendMesh(Mesh& destination, const Mesh& source);
Mesh MergeMeshes(std::initializer_list<const Mesh*> meshes);

Mesh Combine(const TranslationGizmo& gizmo);
Mesh Combine(const RotationGizmo& gizmo);
Mesh Combine(const ScaleGizmo& gizmo);

Color AxisColorX();
Color AxisColorY();
Color AxisColorZ();
Color ScreenAxisColor();
Color ScreenSpaceColor();
Color ArcballColor();
Color HighlightColor();
