#include "EditorSession.h"

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

FMatrix EditorSession::GetProjectionMatrix() const
{
    return FMatrix(
        Camera.Position.GetX() / AspectRatio, 0.0f, 0.0f, 0.0f,
        0.0f, Camera.Position.GetY(), 0.0f, 0.0f,
        0.0f, 0.0f, FarZ / FarZ - NearZ, -FarZ * NearZ / (FarZ - NearZ),
        0.0f, 0.0f, 0.0f, 1.0f
    );
}
