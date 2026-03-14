#include "ControlPanelWindow.h"
#include "Core/Core.h"
#include "Object/Scene/Scene.h"
#include "Object/Actor/Actor.h"
#include "Object/Class.h"
#include "Camera/Camera.h"
#include "Component/SphereComponent.h"
#include "Component/CubeComponent.h"
#include "Debug/EngineLog.h"

#include <filesystem>

void CControlPanelWindow::Render(CCore* Core, AActor*& SelectedActor)
{
	ImGui::Begin("Jungle Control Panel");

	if (!Core || !Core->GetScene())
	{
		ImGui::End();
		return;
	}

	// Camera
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

	// Spawn
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

		UE_LOG("Spawned %s: %s", SpawnTypes[SpawnTypeIndex], Name.c_str());

		SelectedActor = NewActor;
		Core->SetSelectedActor(SelectedActor);
	}

	// Scene Save/Load
	ImGui::SeparatorText("Scene");

	static char SceneName[128] = "NewScene";
	ImGui::InputText("Scene Name", SceneName, IM_ARRAYSIZE(SceneName));

	if (ImGui::Button("Save"))
	{
		FString Path = FString("../Assets/Scenes/") + SceneName + ".json";
		Core->GetScene()->SaveSceneToFile(Path);
		UE_LOG("Scene saved: %s", SceneName);
	}

	ImGui::Spacing();

	static TArray<FString> SceneFiles;
	static int SelectedSceneIndex = -1;

	if (ImGui::Button("Refresh List"))
	{
		SceneFiles.clear();
		SelectedSceneIndex = -1;
		const FString ScenesDir = "../Assets/Scenes";
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
			SelectedActor = nullptr;
			Core->SetSelectedActor(nullptr);
			Core->GetScene()->ClearActors();

			FString Path = FString("../Assets/Scenes/") + SceneFiles[SelectedSceneIndex] + ".json";
			Core->GetScene()->LoadSceneFromFile(Path);
			UE_LOG("Scene loaded: %s", SceneFiles[SelectedSceneIndex].c_str());
		}
	}

	ImGui::End();
}
