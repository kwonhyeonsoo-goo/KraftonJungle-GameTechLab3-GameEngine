#include "ControlPanelWindow.h"
#include "imgui.h"
#include "Core/Core.h"
#include "Renderer/Renderer.h"
#include "Object/Scene/Scene.h"
#include "Object/Actor/Actor.h"
#include "Object/Actor/CubeActor.h"
#include "Object/Actor/SphereActor.h"
#include "Camera/Camera.h"
#include "Core/Paths.h"
#include "Debug/EngineLog.h"
#include <filesystem>

void CControlPanelWindow::Render(CCore* Core, AActor*& SelectedActor)
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
			bool bRotChanged = false;
			bRotChanged |= ImGui::DragFloat("Yaw", &CamYaw, 0.5f);
			bRotChanged |= ImGui::DragFloat("Pitch", &CamPitch, 0.5f, -89.0f, 89.0f);
			if (bRotChanged)
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

		static int32 SpawnTypeIndex = 0;
		const char* SpawnTypes[] = { "Cube", "Sphere" };
		ImGui::Combo("Type", &SpawnTypeIndex, SpawnTypes, IM_ARRAYSIZE(SpawnTypes));

		if (ImGui::Button("Spawn"))
		{
			UScene* Scene = Core->GetScene();
			static int32 SpawnCount = 0;
			FString Name = FString(SpawnTypes[SpawnTypeIndex]) + "_Spawned_" + std::to_string(SpawnCount++);

			AActor* NewActor = nullptr;
			if (SpawnTypeIndex == 0)
				NewActor = Scene->SpawnActor<ACubeActor>(Name);
			else
				NewActor = Scene->SpawnActor<ASphereActor>(Name);

			SelectedActor = NewActor;
			UE_LOG("Spawned %s: %s", SpawnTypes[SpawnTypeIndex], Name.c_str());
		}

		ImGui::SameLine();

		AActor* Selected = SelectedActor;
		if (!Selected)
			ImGui::BeginDisabled();

		if (ImGui::Button("Delete"))
		{
			FString Name = Selected->GetName();
			Core->GetScene()->DestroyActor(Selected);
			SelectedActor = nullptr;
			UE_LOG("Deleted actor: %s", Name.c_str());
		}

		if (!Selected)
			ImGui::EndDisabled();

		ImGui::SeparatorText("Scene");

		static char SceneName[128] = "NewScene";
		ImGui::InputText("Scene Name", SceneName, IM_ARRAYSIZE(SceneName));

		if (ImGui::Button("Save"))
		{
			FString Path = FPaths::SceneDir() + SceneName + ".json";
			Core->GetScene()->SaveSceneToFile(Path);
			UE_LOG("Scene saved: %s", SceneName);
		}

		ImGui::Spacing();

		if (ImGui::Button("Refresh List"))
		{
			SceneFiles.clear();
			SelectedSceneIndex = -1;
			const FString ScenesDir = FPaths::SceneDir();
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
				for (int32 i = 0; i < static_cast<int32>(SceneFiles.size()); ++i)
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
				SelectedActor = nullptr;
				Core->GetScene()->ClearActors();

				FString Path = FPaths::SceneDir() + SceneFiles[SelectedSceneIndex] + ".json";
				Core->GetScene()->LoadSceneFromFile(Path, Core->GetRenderer()->GetDevice());
				UE_LOG("Scene loaded: %s", SceneFiles[SelectedSceneIndex].c_str());
			}
		}
	}

	ImGui::End();
}
