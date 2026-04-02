#include "ControlPanelWindow.h"
#include "imgui.h"
#include "Core/Core.h"
#include "World/WorldContext.h"
#include "World/Level.h"
#include "Camera/Camera.h"
#include "Actor/Actor.h"
#include "Actor/AttachTestActor.h"

#include "Actor/StaticMeshActor.h"
#include "Actor/SubUVActor.h"
#include "Actor/SkySphereActor.h"
#include "Actor/TextActor.h"
#include "Component/TextComponent.h"
#include "UI/EditorViewportClient.h"

namespace
{
	const char* GetLevelTypeLabel(ELevelType LevelType)
	{
		switch (LevelType)
		{
		case ELevelType::Game:
			return "Game";
		case ELevelType::Editor:
			return "Editor";
		case ELevelType::PIE:
			return "PIE";
		case ELevelType::Inactive:
			return "Inactive";
		default:
			return "Unknown";
		}
	}
}

void FControlPanelWindow::Render(FCore* Core, FEditorViewportClient* ActiveViewportClient)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	const bool bOpen = ImGui::Begin("Control Panel");
	ImGui::PopStyleVar();

	if (!bOpen)
	{
		ImGui::End();
		return;
	}

	if (Core && Core->GetLevel())
	{
	//	const FWorldContext* ActiveWorldContext = Core->GetActiveWorldContext();
	//	FCamera* Camera = ActiveViewportClient ? ActiveViewportClient->GetCamera() : nullptr;

	//	ImGui::SeparatorText("Viewport");
	//	if (ActiveViewportClient)
	//	{
	//		ImGui::Text("Active: %s", ActiveViewportClient->GetViewportLabel());
	//		ImGui::Text("World: %s", GetLevelTypeLabel(ActiveViewportClient->GetWorldType()));
	//	}
	//	else
	//	{
	//		ImGui::TextUnformatted("Active: None");
	//	}

	//	if (ActiveWorldContext)
	//	{
	//		ImGui::Text("Scene Context: %s", ActiveWorldContext->ContextName.c_str());
	//	}

	//	ImGui::SeparatorText("Camera");
	//	if (Camera)
	//	{
	//		float Sensitivity = Camera->GetMouseSensitivity();
	//		if (ImGui::SliderFloat("Mouse Sensitivity", &Sensitivity, 0.01f, 1.0f))
	//		{
	//			Camera->SetMouseSensitivity(Sensitivity);
	//		}

	//		float Speed = Camera->GetSpeed();
	//		if (ImGui::SliderFloat("Move Speed", &Speed, 0.1f, 20.0f))
	//		{
	//			Camera->SetSpeed(Speed);
	//		}

	//		const FVector CameraPosition = Camera->GetPosition();
	//		float Position[3] = { CameraPosition.X, CameraPosition.Y, CameraPosition.Z };
	//		if (ImGui::DragFloat3("Position", Position, 0.1f))
	//		{
	//			Camera->SetPosition({ Position[0], Position[1], Position[2] });
	//		}

	//		float CameraYaw = Camera->GetYaw();
	//		float CameraPitch = Camera->GetPitch();
	//		bool bRotationChanged = false;
	//		bRotationChanged |= ImGui::DragFloat("Yaw", &CameraYaw, 0.5f);
	//		bRotationChanged |= ImGui::DragFloat("Pitch", &CameraPitch, 0.5f, -89.0f, 89.0f);
	//		if (bRotationChanged)
	//		{
	//			Camera->SetRotation(CameraYaw, CameraPitch);
	//		}

	//		int ProjectionModeIndex = (Camera->GetProjectionMode() == ECameraProjectionMode::Orthographic) ? 1 : 0;
	//		const char* ProjectionModes[] = { "Perspective", "Orthographic" };
	//		if (ImGui::Combo("Projection", &ProjectionModeIndex, ProjectionModes, IM_ARRAYSIZE(ProjectionModes)))
	//		{
	//			Camera->SetProjectionMode(
	//				ProjectionModeIndex == 0
	//				? ECameraProjectionMode::Perspective
	//				: ECameraProjectionMode::Orthographic);
	//		}

	//		if (Camera->IsOrthographic())
	//		{
	//			float OrthoWidth = Camera->GetOrthoWidth();
	//			if (ImGui::DragFloat("Ortho Width", &OrthoWidth, 0.5f, 1.0f, 1000.0f))
	//			{
	//				Camera->SetOrthoWidth(OrthoWidth);
	//			}
	//		}
	//		else
	//		{
	//			float CameraFOV = Camera->GetFOV();
	//			if (ImGui::SliderFloat("FOV", &CameraFOV, 10.0f, 120.0f))
	//			{
	//				Camera->SetFOV(CameraFOV);
	//			}
	//		}
	//	}

		ImGui::SeparatorText("Spawn");

		static int32 SpawnTypeIndex = 0;
		const char* SpawnTypes[] = { "Cube", "Sphere", "Plane", "AttachTest", "SubUV", "Text", "SkySphere", "StaticMesh" };
		ImGui::Combo("Type", &SpawnTypeIndex, SpawnTypes, IM_ARRAYSIZE(SpawnTypes));

		static char SpawnTextBuffer[256] = "Text";
		if (SpawnTypeIndex == 5)
		{
			ImGui::InputText("Text", SpawnTextBuffer, IM_ARRAYSIZE(SpawnTextBuffer));
		}

		if (ImGui::Button("Spawn"))
		{
			ULevel* Level = Core->GetLevel();
			const FString Name = SpawnTypes[SpawnTypeIndex];

			AActor* NewActor = nullptr;
			ID3D11Device* Device = nullptr;
			// 0:Cube, 1:Sphere, 2:Plane, 7:StaticMesh 모두 AStaticMeshActor로 통합 스폰
			if (SpawnTypeIndex == 0 || SpawnTypeIndex == 1 || SpawnTypeIndex == 2 || SpawnTypeIndex == 7)
			{
				NewActor = Level->SpawnActor<AStaticMeshActor>(Name);
				if (NewActor)
				{
					AStaticMeshActor* SMActor = static_cast<AStaticMeshActor*>(NewActor);

					// 주의: 현재 구조에서 ID3D11Device를 획득하는 코드(예: Core->GetDevice() 등)로 수정해 주셔야 합니다.
				

					if (SpawnTypeIndex == 0)
					{
						SMActor->LoadStaticMesh(Device, "Engine/BasicShapes/Cube");
					}
					else if (SpawnTypeIndex == 1)
					{
						SMActor->LoadStaticMesh(Device, "Engine/BasicShapes/Sphere");
					}
					else if (SpawnTypeIndex == 2)
					{
						SMActor->LoadStaticMesh(Device, "Engine/BasicShapes/Plane");
					}
					else if (SpawnTypeIndex == 7)
					{
						// 외부 StaticMesh 스폰 로직 (필요시 파일 브라우저 연동 등)
						SMActor->LoadStaticMesh(Device, "Engine/BasicShapes/Cube");
					}
				}
			}
			else if (SpawnTypeIndex == 3)
			{
				NewActor = Level->SpawnActor<AAttachTestActor>(Name);
			}
			else if (SpawnTypeIndex == 4)
			{
				NewActor = Level->SpawnActor<ASubUVActor>(Name);
			}
			else if (SpawnTypeIndex == 5)
			{
				NewActor = Level->SpawnActor<ATextActor>(Name);
				if (NewActor)
				{
					ATextActor* TextActor = static_cast<ATextActor*>(NewActor);
					if (UTextComponent* TextComponent = TextActor->GetTextComponent())
					{
						TextComponent->SetText(SpawnTextBuffer[0] != '\0' ? SpawnTextBuffer : "Text");
					}
				}
			}
			else if (SpawnTypeIndex == 6)
			{
				NewActor = Level->SpawnActor<ASkySphereActor>(Name);
				if (NewActor)
				{
					ASkySphereActor* SkyActor = static_cast<ASkySphereActor*>(NewActor);

		
					SkyActor->LoadSkyMesh(Device);
				}
			}

			if (NewActor && !NewActor->IsA<ASkySphereActor>())
			{
				Core->SetSelectedActor(NewActor);
			}
		}

		ImGui::SameLine();
		AActor* SelectedActor = Core->GetSelectedActor();
		if (!SelectedActor)
		{
			ImGui::BeginDisabled();
		}

		if (ImGui::Button("Delete"))
		{
			Core->GetLevel()->DestroyActor(SelectedActor);
			Core->SetSelectedActor(nullptr);
		}

		if (!SelectedActor)
		{
			ImGui::EndDisabled();
		}
	}

	ImGui::End();
}
