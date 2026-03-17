#include "ControlPanelWindow.h"

#include "imgui.h"
#include "Core/Core.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"
#include "Actor/Actor.h"
#include "Actor/AttachTestActor.h"
#include "Actor/CubeActor.h"
#include "Actor/SphereActor.h"
#include "Object/ObjectFactory.h"
#include "Camera/Camera.h"
#include "Core/Paths.h"
#include "Debug/EngineLog.h"

#include <filesystem>

namespace
{
	const char* GetSceneTypeLabel(ESceneType SceneType)
	{
		switch (SceneType)
		{
		case ESceneType::Game:
			return "Game";
		case ESceneType::Editor:
			return "Editor";
		case ESceneType::PIE:
			return "PIE";
		case ESceneType::Preview:
			return "Preview";
		case ESceneType::Inactive:
			return "Inactive";
		default:
			return "Unknown";
		}
	}
}

void CControlPanelWindow::Render(CCore* Core)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	const bool bOpen = ImGui::Begin("Control Panel");
	ImGui::PopStyleVar();

	if (!bOpen)
	{
		ImGui::End();
		return;
	}

	if (Core && Core->GetScene())
	{
		const FSceneContext* ActiveSceneContext = Core->GetActiveSceneContext();
		const TArray<std::unique_ptr<FEditorSceneContext>>& PreviewSceneContexts = Core->GetPreviewSceneContexts();
		const bool bPreviewActive = ActiveSceneContext && ActiveSceneContext->SceneType == ESceneType::Preview;

		ImGui::SeparatorText("World");

		if (ActiveSceneContext)
		{
			ImGui::Text("Active: %s", ActiveSceneContext->ContextName.c_str());
			ImGui::Text("Type: %s", GetSceneTypeLabel(ActiveSceneContext->SceneType));
		}

		if (ImGui::Button("Editor Scene"))
		{
			Core->ActivateEditorScene();
		}

		ImGui::SameLine();

		if (PreviewSceneContexts.empty())
		{
			ImGui::BeginDisabled();
			ImGui::Button("Preview Scene");
			ImGui::EndDisabled();
		}
		else if (ImGui::Button("Preview Scene"))
		{
			Core->ActivatePreviewScene(PreviewSceneContexts.front()->ContextName);
		}

		if (bPreviewActive)
		{
			ImGui::TextUnformatted("Preview scene is editor-only. Scene save/load is disabled.");
		}

		if (CCamera* Camera = Core->GetScene()->GetCamera())
		{
			ImGui::SeparatorText("Camera");

			const FVector CameraPosition = Camera->GetPosition();
			float Position[3] = { CameraPosition.X, CameraPosition.Y, CameraPosition.Z };
			if (ImGui::DragFloat3("Position", Position, 0.1f))
			{
				Camera->SetPosition({ Position[0], Position[1], Position[2] });
			}

			float CameraYaw = Camera->GetYaw();
			float CameraPitch = Camera->GetPitch();
			bool bRotationChanged = false;
			bRotationChanged |= ImGui::DragFloat("Yaw", &CameraYaw, 0.5f);
			bRotationChanged |= ImGui::DragFloat("Pitch", &CameraPitch, 0.5f, -89.0f, 89.0f);
			if (bRotationChanged)
			{
				Camera->SetRotation(CameraYaw, CameraPitch);
			}

			float CameraFOV = Camera->GetFOV();
			if (ImGui::SliderFloat("FOV", &CameraFOV, 10.0f, 120.0f))
			{
				Camera->SetFOV(CameraFOV);
			}
		}

		ImGui::SeparatorText("Spawn");

		static int32 SpawnTypeIndex = 0;
		const char* SpawnTypes[] = { "Cube", "Sphere", "AttachTest" };
		ImGui::Combo("Type", &SpawnTypeIndex, SpawnTypes, IM_ARRAYSIZE(SpawnTypes));

		if (ImGui::Button("Spawn"))
		{
			UScene* Scene = Core->GetScene();
			static int32 SpawnCount = 0;
			const FString Name = FString(SpawnTypes[SpawnTypeIndex]) + "_Spawned_" + std::to_string(SpawnCount++);

			AActor* NewActor = nullptr;
			if (SpawnTypeIndex == 0)
			{
				NewActor = Scene->SpawnActor<ACubeActor>(Name);
			}
			else if (SpawnTypeIndex == 1)
			{
				NewActor = Scene->SpawnActor<ASphereActor>(Name);
			}
			else if (SpawnTypeIndex == 2)
			{
				NewActor = Scene->SpawnActor<AAttachTestActor>(Name);
			}

			Core->SetSelectedActor(NewActor);
			UE_LOG("Spawned %s: %s", SpawnTypes[SpawnTypeIndex], Name.c_str());
		}

		ImGui::SameLine();

		AActor* SelectedActor = Core->GetSelectedActor();
		if (!SelectedActor)
		{
			ImGui::BeginDisabled();
		}

		if (ImGui::Button("Delete"))
		{
			const FString Name = SelectedActor->GetName();
			Core->GetScene()->DestroyActor(SelectedActor);
			Core->SetSelectedActor(nullptr);
			UE_LOG("Deleted actor: %s", Name.c_str());
		}

		if (!SelectedActor)
		{
			ImGui::EndDisabled();
		}

		ImGui::SeparatorText("Scene");

		if (ImGui::Button("Clear Scene"))
		{
			Core->SetSelectedActor(nullptr);
			Core->GetScene()->ClearActors();
			UE_LOG("Scene cleared: all actors destroyed");
		}

		ImGui::Spacing();

		static char SceneName[128] = "NewScene";
		ImGui::InputText("Scene Name", SceneName, IM_ARRAYSIZE(SceneName));

		ImGui::BeginDisabled(bPreviewActive);

		if (ImGui::Button("Save"))
		{
			const FString Path = FPaths::SceneDir() + SceneName + ".json";
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
				for (int32 Index = 0; Index < static_cast<int32>(SceneFiles.size()); ++Index)
				{
					const bool bSelected = (SelectedSceneIndex == Index);
					if (ImGui::Selectable(SceneFiles[Index].c_str(), bSelected))
					{
						SelectedSceneIndex = Index;
					}
				}
				ImGui::EndListBox();
			}

			if (SelectedSceneIndex >= 0 && ImGui::Button("Load"))
			{
				Core->SetSelectedActor(nullptr);
				Core->GetScene()->ClearActors();

				const FString Path = FPaths::SceneDir() + SceneFiles[SelectedSceneIndex] + ".json";
				Core->GetScene()->LoadSceneFromFile(Path, Core->GetRenderer()->GetDevice());
				UE_LOG("Scene loaded: %s", SceneFiles[SelectedSceneIndex].c_str());
			}
		}

		ImGui::EndDisabled();
	}

	ImGui::End();
}
