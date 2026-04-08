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
#include "Actor/Pawn.h"
#include "Actor/Controller.h"
#include "Actor/StaticMeshActor.h"
#include "Actor/SubUVActor.h"
#include "Actor/SkySphereActor.h"
#include "Actor/TextActor.h"

#include "Component/CameraComponent.h"
#include "Component/TextRenderComponent.h"
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
		const char* SpawnTypes[] = { "Cube", "Sphere", "Plane", "AttachTest", "SubUV", "Text", "SkySphere", "StaticMesh", "Pawn", "Controller" };
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
			switch (SpawnTypeIndex)
			{
			case 0:
			case 1:
			case 2:
			case 7:
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
				break;
			}
			case 3:
			{
				NewActor = Level->SpawnActor<AAttachTestActor>(Name);
				break;
			}
			case 4:
			{
				NewActor = Level->SpawnActor<ASubUVActor>(Name);
				break;
			}
			case 5:
			{
				NewActor = Level->SpawnActor<ATextActor>(Name);
				if (NewActor)
				{
					ATextActor* TextActor = static_cast<ATextActor*>(NewActor);
					if (UTextRenderComponent* TextComponent = TextActor->GetTextComponent())
					{
						TextComponent->SetText(SpawnTextBuffer[0] != '\0' ? SpawnTextBuffer : "Text");
					}
				}
				break;
			}
			case 6:
			{
				NewActor = Level->SpawnActor<ASkySphereActor>(Name);
				if (NewActor)
				{
					ASkySphereActor* SkyActor = static_cast<ASkySphereActor*>(NewActor);
					SkyActor->LoadSkyMesh(Device);
				}
				break;
			}
			case 8:
			{
				NewActor = Level->SpawnActor<APawn>(Name);
				UCameraComponent* PawnCamera = FObjectFactory::ConstructObject<UCameraComponent>(NewActor, Name + "_Camera");
				PawnCamera->SetRelativeTransform(FTransform::Identity);
				NewActor->AddOwnedComponent(PawnCamera);
				break;
			}
			case 9:
			{
				NewActor = Level->SpawnActor<AController>(Name);
				break;
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
