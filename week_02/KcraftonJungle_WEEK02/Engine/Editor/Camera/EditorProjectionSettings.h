#pragma once

#include "../../Foundation/Math/FMatrix.h"

enum class EEditorProjectionMode
{
    Perspective,
    Orthographic,
};

struct FEditorProjectionSettings
{
    float FovY = 60.0f;        // degree
    float AspectRatio = 16.0f / 9.0f;
    float NearZ = 0.1f;
    float FarZ = 1000.0f;

    float OrthoHeight = 10.0f;
    EEditorProjectionMode Mode = EEditorProjectionMode::Perspective;

    FMatrix BuildPerspectiveMatrix() const;
    FMatrix BuildOrthographicMatrix() const;
    FMatrix BuildProjectionMatrix() const;
};