#include "EditorViewport.h"

FMatrix FEditorViewport::GetViewMatrix() const
{
    return Camera.GetViewMatrix();
}

FMatrix FEditorViewport::GetProjectionMatrix() const
{
    return Projection.BuildProjectionMatrix();
}

FMatrix FEditorViewport::GetViewProjMatrix() const
{
    return GetViewMatrix() * GetProjectionMatrix();
}

FMatrix FEditorViewport::GetOrthographicMatrix() const
{
    return Projection.BuildOrthographicMatrix();
}

FMatrix FEditorViewport::GetViewOrthoMatrix() const
{
    return GetViewMatrix() * GetOrthographicMatrix();
}