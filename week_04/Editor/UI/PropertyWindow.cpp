#include "PropertyWindow.h"
#include "imgui.h"

void CPropertyWindow::SetTarget(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
	EditLocation = Location;
	EditRotation = Rotation;
	EditScale = Scale;
	bModified = false;
}

void CPropertyWindow::Render()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	ImGui::PopStyleVar();
	if (!ImGui::Begin("Properties"))
	{
		ImGui::End();
		return;
	}

	bModified = false;

	float Loc[3] = { EditLocation.X, EditLocation.Y, EditLocation.Z };
	float Rot[3] = { EditRotation.X, EditRotation.Y, EditRotation.Z };
	float Scl[3] = { EditScale.X,    EditScale.Y,    EditScale.Z };

	if (ImGui::DragFloat3("Location", Loc, 0.1f))
	{
		EditLocation = { Loc[0], Loc[1], Loc[2] };
		bModified = true;
	}
	if (ImGui::DragFloat3("Rotation", Rot, 0.5f))
	{
		EditRotation = { Rot[0], Rot[1], Rot[2] };
		bModified = true;
	}
	if (ImGui::DragFloat3("Scale", Scl, 0.01f, 0.001f, 100.0f))
	{
		EditScale = { Scl[0], Scl[1], Scl[2] };
		bModified = true;
	}

	ImGui::End();
}