#pragma once

struct InputState;
struct FVector;
struct FEditorViewport;

class FEditorCameraController
{
public:
    static void ProcessFreeCameraInput(FEditorViewport& viewport,
        const InputState& input,
        float deltaTime);

    static void FocusPoint(FEditorViewport& viewport,
        const FVector& target,
        float distance);
};