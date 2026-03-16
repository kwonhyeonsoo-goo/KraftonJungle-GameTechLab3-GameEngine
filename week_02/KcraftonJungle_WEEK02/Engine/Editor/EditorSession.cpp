#include "EditorSession.h"
#include <cmath>
#include "../Platform/InputRouter.h"

FMatrix CameraState::GetViewMatrix() const
{
    const FVector Forward = GetForwardVector();
    return FMatrix::LookAt(Position, Position + Forward, FVector(0.0f, 0.0f, 1.0f));
}

FVector CameraState::GetForwardVector() const
{
    const float cp = std::cos(Pitch);
    const float sp = std::sin(Pitch);
    const float cy = std::cos(Yaw);
    const float sy = std::sin(Yaw);

    return FVector(cp * cy, cp * sy, sp);
}

FVector CameraState::GetRightVector() const
{
    const float cy = std::cos(Yaw);
    const float sy = std::sin(Yaw);
    return FVector(-sy, cy, 0.0f);
}

EditorSession::EditorSession()
{
    Selection = SelectionSet();
    Tools = ToolContext();
    Camera = CameraState();

    FovY = 60.0f;
    AspectRatio = 16.0f / 9.0f;
    NearZ = 0.1f;
    FarZ = 1000.0f;
}

void EditorSession::ProcessCameraInput(const InputState& input, float deltaTime)
{
    if (input.Keys['W'])
        Camera.Position += Camera.GetForwardVector() * Camera.MoveSpeed * deltaTime;
    if (input.Keys['S'])
        Camera.Position -= Camera.GetForwardVector() * Camera.MoveSpeed * deltaTime;
    if (input.Keys['D'])
        Camera.Position += Camera.GetRightVector() * Camera.MoveSpeed * deltaTime;
    if (input.Keys['A'])
        Camera.Position -= Camera.GetRightVector() * Camera.MoveSpeed * deltaTime;
    
    if (input.Keys['Q'])
        Camera.Position -= Camera.GetForwardVector().Cross(Camera.GetRightVector()) * Camera.MoveSpeed * deltaTime;
    if (input.Keys['E'])
        Camera.Position += Camera.GetForwardVector().Cross(Camera.GetRightVector()) * Camera.MoveSpeed * deltaTime;

    if (input.MouseButtons[1]) {
        Camera.Yaw  += input.MouseDeltaX * Camera.RotSpeed * deltaTime;
        Camera.Pitch -= input.MouseDeltaY * Camera.RotSpeed * deltaTime;

        Camera.Yaw = fmod(Camera.Yaw, 360.f);
        Camera.Pitch = fmod(Camera.Pitch, 360.f);
    }
}

FMatrix EditorSession::GetProjectionMatrix() const
{
    constexpr float DegToRad = 3.14159265358979323846f / 180.0f;
    return FMatrix::Perspective(FovY * DegToRad, AspectRatio, NearZ, FarZ);
}

FMatrix EditorSession::GetOrthogonalMatrix() const
{
    constexpr float DegToRad = 3.14159265358979323846f / 180.0f;
    return FMatrix::Orthographic(OrthoHeight, OrthoHeight * AspectRatio, NearZ, FarZ);
}

FMatrix EditorSession::GetViewProjMatrix() const
{
    return Camera.GetViewMatrix() * GetProjectionMatrix();
}

FMatrix EditorSession::GetViewOrthoMatrix() const
{
    return Camera.GetViewMatrix() * GetOrthogonalMatrix();
}

float EditorSession::ComputeGizmoScale(const FVector& gizmoPos, float viewportHeight) const
{
    constexpr float DegToRad = 3.14159265358979323846f / 180.0f;
    constexpr float DesiredPixels = 96.0f;

    if (bOrthoMode)
    {
        return DesiredPixels * (OrthoHeight / viewportHeight);
    }

    const FVector toGizmo = gizmoPos - Camera.Position;
    const float depth = (std::max)(0.001f, toGizmo.Dot(Camera.GetForwardVector()));

    const float fovYRad = FovY * DegToRad;
    const float worldPerPixel =
        2.0f * depth * std::tan(fovYRad * 0.5f) / viewportHeight;

    return DesiredPixels * worldPerPixel;
}