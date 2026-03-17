#include "EditorCameraController.h"

#include <cmath>
#include "../Viewport/EditorViewport.h"
#include "../../Platform/InputRouter.h"

namespace
{
    constexpr float Pi = 3.14159265358979323846f;
    constexpr float TwoPi = 6.28318530717958647692f;
    constexpr float HalfPi = 1.57079632679489661923f;
    constexpr float PitchLimit = HalfPi - 0.001f;

    float WrapAngleRad(float A)
    {
        A = std::fmod(A, TwoPi);
        if (A < 0.0f)
            A += TwoPi;
        return A;
    }

    float ClampFloat(float V, float MinV, float MaxV)
    {
        return (V < MinV) ? MinV : ((V > MaxV) ? MaxV : V);
    }
}

void FEditorCameraController::ProcessFreeCameraInput(FEditorViewport& Viewport,
    const InputState& Input,
    float DeltaTime)
{
    FEditorCameraState& Cam = Viewport.Camera;

    const FVector Forward = Cam.GetForwardVector();
    const FVector Right = Cam.GetRightVector();
    const FVector Up = Cam.GetUpVector();

    const float MoveStep = Cam.MoveSpeed * DeltaTime;

    if (Input.Keys['W'])
        Cam.Position += Forward * MoveStep;
    if (Input.Keys['S'])
        Cam.Position -= Forward * MoveStep;
    if (Input.Keys['D'])
        Cam.Position += Right * MoveStep;
    if (Input.Keys['A'])
        Cam.Position -= Right * MoveStep;
    if (Input.Keys['Q'])
        Cam.Position -= Up * MoveStep;
    if (Input.Keys['E'])
        Cam.Position += Up * MoveStep;

    if (Input.MouseButtons[1])
    {
        Cam.Yaw += Input.MouseDeltaX * Cam.RotSpeed * DeltaTime;
        Cam.Pitch -= Input.MouseDeltaY * Cam.RotSpeed * DeltaTime;

        Cam.Yaw = WrapAngleRad(Cam.Yaw);
        Cam.Pitch = ClampFloat(Cam.Pitch, -PitchLimit, PitchLimit);
    }
}

void FEditorCameraController::FocusPoint(FEditorViewport& Viewport,
    const FVector& Target,
    float Distance)
{
    FEditorCameraState& Cam = Viewport.Camera;
    const FVector Forward = Cam.GetForwardVector().Normalized();
    Cam.Position = Target - Forward * Distance;
}