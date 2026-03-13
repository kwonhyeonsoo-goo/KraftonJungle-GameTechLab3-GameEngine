#include "PropertyWindow.h"
// #include <imgui.h>

void CPropertyWindow::Render()
{
    // TODO: ImGui 창 — Location, Rotation, Scale 수치 입력
    bModified = false;
}

void CPropertyWindow::SetTarget(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
    EditLocation = Location;
    EditRotation = Rotation;
    EditScale = Scale;
}
