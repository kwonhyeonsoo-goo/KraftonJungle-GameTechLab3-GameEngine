#include "EditorSession.h"

#include "../Platform/InputRouter.h"
#include "../World/USceneComponent.h"
#include "./Camera/EditorCameraController.h"

EditorSession::EditorSession()
{
    Selection = SelectionSet();
    Tools = ToolContext();

    FEditorViewport DefaultViewport;
    DefaultViewport.Name = "Perspective";
    DefaultViewport.Type = EEditorViewportType::Perspective;

    DefaultViewport.Camera = FEditorCameraState();
    DefaultViewport.Projection = FEditorProjectionSettings();

    Viewports.push_back(DefaultViewport);
    ActiveViewportIndex = 0;
}

FEditorViewport& EditorSession::GetActiveViewport()
{
    return Viewports[ActiveViewportIndex];
}

const FEditorViewport& EditorSession::GetActiveViewport() const
{
    return Viewports[ActiveViewportIndex];
}

FEditorCameraState& EditorSession::GetActiveCamera()
{
    return GetActiveViewport().Camera;
}

const FEditorCameraState& EditorSession::GetActiveCamera() const
{
    return GetActiveViewport().Camera;
}

const TArray<FEditorViewport>& EditorSession::GetViewports() const
{
    return Viewports;
}

void EditorSession::SetActiveViewportIndex(int32 Index)
{
    ActiveViewportIndex = Index;
}

int32 EditorSession::GetActiveViewportIndex() const
{
    return ActiveViewportIndex;
}

void EditorSession::ProcessCameraInput(const InputState& Input, float DeltaTime)
{
    FEditorCameraController::ProcessFreeCameraInput(GetActiveViewport(), Input, DeltaTime);
}

FMatrix EditorSession::GetProjectionMatrix() const
{
    return GetActiveViewport().GetProjectionMatrix();
}

FMatrix EditorSession::GetOrthogonalMatrix() const
{
    return GetActiveViewport().GetOrthographicMatrix();
}

FMatrix EditorSession::GetViewProjMatrix() const
{
    return GetActiveViewport().GetViewProjMatrix();
}

FMatrix EditorSession::GetViewOrthoMatrix() const
{
    return GetActiveViewport().GetViewOrthoMatrix();
}

void EditorSession::FocusObject(const USceneComponent* Comp, float Distance)
{
    if (Comp == nullptr)
        return;

    FEditorCameraController::FocusPoint(
        GetActiveViewport(),
        Comp->GetTransform().Location,
        Distance);
}