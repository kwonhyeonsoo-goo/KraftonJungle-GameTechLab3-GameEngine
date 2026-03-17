#pragma once

#include "../../Foundation/Core/CoreTypes.h"
#include "../Camera/EditorCameraState.h"
#include "../Camera/EditorProjectionSettings.h"

enum class EEditorViewportType
{
    Perspective,
    Top,
    Front,
    Right,
    Custom,
};

struct FEditorViewport
{
    FString Name = "Perspective";
    EEditorViewportType Type = EEditorViewportType::Perspective;

    bool bVisible = true;
    bool bReceivesInput = true;

    FEditorCameraState       Camera;
    FEditorProjectionSettings Projection;

    FMatrix GetViewMatrix() const;
    FMatrix GetProjectionMatrix() const;
    FMatrix GetViewProjMatrix() const;
    FMatrix GetOrthographicMatrix() const;
    FMatrix GetViewOrthoMatrix() const;
};