#include "EditorProjectionSettings.h"

namespace
{
    constexpr float Pi = 3.14159265358979323846f;
    constexpr float DegToRad = Pi / 180.0f;
}

FMatrix FEditorProjectionSettings::BuildPerspectiveMatrix() const
{
    return FMatrix::Perspective(FovY * DegToRad, AspectRatio, NearZ, FarZ);
}

FMatrix FEditorProjectionSettings::BuildOrthographicMatrix() const
{
    return FMatrix::Orthographic(OrthoHeight, OrthoHeight * AspectRatio, NearZ, FarZ);
}

FMatrix FEditorProjectionSettings::BuildProjectionMatrix() const
{
    return (Mode == EEditorProjectionMode::Orthographic)
        ? BuildOrthographicMatrix()
        : BuildPerspectiveMatrix();
}