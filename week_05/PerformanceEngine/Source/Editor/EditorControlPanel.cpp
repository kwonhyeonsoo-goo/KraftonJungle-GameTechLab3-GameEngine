#include "EditorControlPanel.h"

#include "Core/Core.h"
#include "Camera/Camera.h"

FEditorControlPanel::FEditorControlPanel(FCore* InCore)
{
	Core = InCore;
}

FEditorControlPanel::~FEditorControlPanel()
{
}

void FEditorControlPanel::Render()
{
	if (ImGui::Begin("Control Panel"))
	{
		FVector CameraLocation = Core->GetCamera()->GetLocation();
		FRotator CameraRotation = Core->GetCamera()->GetRotation().Rotator();
		float CameraFOV = Core->GetCamera()->GetFOV();
		float CameraSpeed = Core->GetCamera()->GetSpeed();
		float CameraSensitivity = Core->GetCamera()->GetSensitivity();

		if (ImGui::DragFloat3("Camera Location", &CameraLocation.X, 0.1f))
		{
			Core->GetCamera()->SetLocation(CameraLocation);
		}
		if (ImGui::DragFloat3("Camera Rotation", &CameraRotation.Roll, 0.1f))
		{
			Core->GetCamera()->SetRotation(CameraRotation);
		}
		if (ImGui::DragFloat3("Camera FOV", &CameraFOV, 1.0f, 179.0f))
		{
			Core->GetCamera()->SetFOV(CameraFOV);
		}

		ImGui::Separator();

		if (ImGui::SliderFloat("Camera Speed", &CameraSpeed, 0.1f, 100.0f))
		{
			Core->GetCamera()->SetSpeed(CameraSpeed);
		}
		if (ImGui::SliderFloat("Camera Sensitivity", &CameraSensitivity, 0.01f, 5.0f))
		{
			Core->GetCamera()->SetSensitivity(CameraSensitivity);
		}

		ImGui::End();
	}
}