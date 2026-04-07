#include "ControlPanelWindow.h"
#include "imgui.h"
#include "Core/Core.h"
#include "FEditorEngine.h"
#include "PIEState.h"
#include "World/World.h"
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
	const char* GetLevelTypeLabel(EWorldType LevelType)
	{
		switch (LevelType)
		{
		case EWorldType::Game:
			return "Game";
		case EWorldType::Editor:
			return "Editor";
		case EWorldType::PIE:
			return "PIE";
		case EWorldType::Inactive:
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

	if (GWorld && GWorld->GetLevel())
	{
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
			ULevel* Level = GWorld->GetLevel();
			const FString Name = SpawnTypes[SpawnTypeIndex];

			AActor* NewActor = nullptr;
			ID3D11Device* Device = GRenderer->GetDevice();
			// 0:Cube, 1:Sphere, 2:Plane, 7:StaticMesh 모두 AStaticMeshActor로 통합 스폰
			if (SpawnTypeIndex == 0 || SpawnTypeIndex == 1 || SpawnTypeIndex == 2 || SpawnTypeIndex == 7)
			{
				NewActor = Level->SpawnActor<AStaticMeshActor>(Name);
				if (NewActor)
				{
					AStaticMeshActor* SMActor = static_cast<AStaticMeshActor*>(NewActor);

				

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
				GEditor->SetSelectedActor(NewActor);
			}
		}

		ImGui::SameLine();
		AActor* SelectedActor = GEditor->GetSelectedActor();
		if (!SelectedActor)
		{
			ImGui::BeginDisabled();
		}

		if (ImGui::Button("Delete"))
		{
			GWorld->GetLevel()->DestroyActor(SelectedActor);
			GEditor->SetSelectedActor(nullptr);
		}

		if (!SelectedActor)
		{
			ImGui::EndDisabled();
		}
	}

	ImGui::End();
}
