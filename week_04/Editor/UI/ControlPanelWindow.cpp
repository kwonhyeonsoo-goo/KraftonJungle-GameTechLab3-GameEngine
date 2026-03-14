#include "ControlPanelWindow.h"
#include "imgui.h"
#include "Core/Core.h"
#include "Object/Scene/Scene.h"
#include "Object/Actor/Actor.h"
#include "Camera/Camera.h"
#include "Component/CubeComponent.h"
#include "Component/SphereComponent.h"
#include <filesystem>


void CControlPanelWindow::Render(CCore* Core)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	bool bOpen = ImGui::Begin("Control Panel");
	ImGui::PopStyleVar();

	if (!bOpen)
	{
		ImGui::End();
		return;
	}

	if (Core && Core->GetScene())
	{
		CCamera* Cam = Core->GetScene()->GetCamera();
		if (Cam)
		{
			ImGui::SeparatorText("Camera");

			FVector CamPos = Cam->GetPosition();
			float Pos[3] = { CamPos.X, CamPos.Y, CamPos.Z };
			if (ImGui::DragFloat3("Position", Pos, 0.1f))
			{
				Cam->SetPosition({ Pos[0], Pos[1], Pos[2] });
			}

			float CamYaw = Cam->GetYaw();
			float CamPitch = Cam->GetPitch();
			bool RotChanged = false;
			RotChanged |= ImGui::DragFloat("Yaw", &CamYaw, 0.5f);
			RotChanged |= ImGui::DragFloat("Pitch", &CamPitch, 0.5f, -89.0f, 89.0f);
			if (RotChanged)
			{
				Cam->SetRotation(CamYaw, CamPitch);
			}

			float CamFOV = Cam->GetFOV();
			if (ImGui::SliderFloat("FOV", &CamFOV, 10.0f, 120.0f))
			{
				Cam->SetFOV(CamFOV);
			}
		}
		ImGui::SeparatorText("Spawn");

		static int SpawnTypeIndex = 0;
		const char* SpawnTypes[] = { "Cube", "Sphere" };
		ImGui::Combo("Type", &SpawnTypeIndex, SpawnTypes, IM_ARRAYSIZE(SpawnTypes));

		if (ImGui::Button("Spawn"))
		{
			UScene* Scene = Core->GetScene();
			static int SpawnCount = 0;
			FString Name = FString(SpawnTypes[SpawnTypeIndex]) + "_Spawned_" + std::to_string(SpawnCount++);
			AActor* NewActor = Scene->SpawnActor<AActor>(Name);

			UActorComponent* Comp = nullptr;
			if (SpawnTypeIndex == 0)
				Comp = new UCubeComponent();
			else
				Comp = new USphereComponent();

			NewActor->AddOwnedComponent(Comp);

			// 새 액터 스폰 후 자동 선택
			Core->SetSelectedActor(NewActor);
		}
		ImGui::SeparatorText("Scene");

		static char SceneName[128] = "NewScene";
		ImGui::InputText("Scene Name", SceneName, IM_ARRAYSIZE(SceneName));

		if (ImGui::Button("Save"))
		{
			FString Path = FString("../Assets/Scenes/") + SceneName + ".json";
			Core->GetScene()->SaveSceneToFile(Path);
		}

		ImGui::Spacing();

		if (ImGui::Button("Refresh List"))
		{
			SceneFiles.clear();
			SelectedSceneIndex = -1;
			const std::string ScenesDir = "../Assets/Scenes";
			if (std::filesystem::exists(ScenesDir))
			{
				for (auto& Entry : std::filesystem::directory_iterator(ScenesDir))
				{
					if (Entry.path().extension() == ".json")
					{
						SceneFiles.push_back(Entry.path().stem().string());
					}
				}
			}
		}
		if (!SceneFiles.empty())
		{
			if (ImGui::BeginListBox("Scenes"))
			{
				for (int i = 0; i < static_cast<int>(SceneFiles.size()); ++i)
				{
					bool bSelected = (SelectedSceneIndex == i);
					if (ImGui::Selectable(SceneFiles[i].c_str(), bSelected))
					{
						SelectedSceneIndex = i;
					}
				}
				ImGui::EndListBox();
			}

			if (SelectedSceneIndex >= 0 && ImGui::Button("Load"))
			{
				Core->SetSelectedActor(nullptr);
				Core->GetScene()->ClearActors();

				FString Path = FString("../Assets/Scenes/") + SceneFiles[SelectedSceneIndex] + ".json";
				Core->GetScene()->LoadSceneFromFile(Path);
			}
		}
	}
	ImGui::End();
}
