#include "ControlPanelWindow.h"
#include "World/WorldContext.h"
#include "imgui.h"
#include "Core/Core.h"
#include "Core/ViewportClient.h"
#include "Renderer/Renderer.h"
#include "World/Level.h"
#include "Actor/Actor.h"
#include "Actor/AttachTestActor.h"
#include "Actor/CubeActor.h"
#include "Actor/SphereActor.h"
#include "Actor/PlaneActor.h"
#include "Actor/SubUVActor.h"
#include "Actor/TextActor.h"
#include "Component/TextComponent.h"
#include "Component/CameraComponent.h"
#include "Object/ObjectFactory.h"
#include "Core/Paths.h"
#include "Debug/EngineLog.h"
#include "Actor/SkySphereActor.h"
#include "Serializer/SceneSerializer.h"
#include <filesystem>

void FControlPanelWindow::Render(FCore* Core)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	const bool bOpen = ImGui::Begin("Control Panel");
	ImGui::PopStyleVar();

	if (!bOpen)
	{
		ImGui::End();
		return;
	}

	if (!Core || !Core->GetLevel())
	{
		ImGui::End();
		return;
	}

	ImGui::SeparatorText("Camera");

	// FCamera 제거 — 포커스된(또는 첫 번째) ViewportClient의 ActiveCamera 사용
	UCameraComponent* Camera = nullptr;
	if (Core->GetViewportClientCount() > 0)
	{
		if (IViewportClient* VP = Core->GetViewportClientAt(0))
		{
			Camera = VP->GetActiveCamera();
		}
	}

	if (Camera)
	{
		// ── 컨트롤 파라미터 (추후 컨트롤러 레이어 분리 예정) ──────────
		float Sensitivity = Camera->GetSensitivity();
		if (ImGui::SliderFloat("Mouse Sensitivity", &Sensitivity, 0.01f, 1.0f))
		{
			Camera->SetSensitivity(Sensitivity);
		}

		float Speed = Camera->GetSpeed();
		if (ImGui::SliderFloat("Move Speed", &Speed, 0.1f, 20.0f))
		{
			Camera->SetSpeed(Speed);
		}

		// ── Transform ──────────────────────────────────────────────────
		FVector CameraPosition = Camera->GetPosition();
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

		// ── 렌즈 파라미터 ──────────────────────────────────────────────
		const bool bIsOrtho = Camera->IsOrthographic();
		int ProjectionModeIndex = bIsOrtho ? 1 : 0;
		const char* ProjectionModes[] = { "Perspective", "Orthographic" };
		if (ImGui::Combo("Projection", &ProjectionModeIndex, ProjectionModes, IM_ARRAYSIZE(ProjectionModes)))
		{
			Camera->SetOrthographic(ProjectionModeIndex == 1);
		}

		if (Camera->IsOrthographic())
		{
			float OrthoWidth = Camera->GetOrthoWidth();
			if (ImGui::DragFloat("Ortho Width", &OrthoWidth, 0.5f, 1.0f, 1000.0f))
			{
				Camera->SetOrthoWidth(OrthoWidth);
			}
		}
		else
		{
			float CameraFOV = Camera->GetFOV();
			if (ImGui::SliderFloat("FOV", &CameraFOV, 10.0f, 120.0f))
			{
				Camera->SetFOV(CameraFOV);
			}
		}
	}

	// ── Spawn ──────────────────────────────────────────────────────────────
	ImGui::SeparatorText("Spawn");

	static int32 SpawnTypeIndex = 0;
	const char* SpawnTypes[] = { "Cube", "Sphere", "Plane", "AttachTest", "SubUV", "Text", "SkySphere" };
	ImGui::Combo("Type", &SpawnTypeIndex, SpawnTypes, IM_ARRAYSIZE(SpawnTypes));

	static char SpawnTextBuffer[256] = "Text";
	if (SpawnTypeIndex == 5)
	{
		ImGui::InputText("Text", SpawnTextBuffer, IM_ARRAYSIZE(SpawnTextBuffer));
	}

	if (ImGui::Button("Spawn"))
	{
		ULevel* Level = Core->GetLevel();
		static int32 SpawnCount = 0;
		const FString Name = FString(SpawnTypes[SpawnTypeIndex]) + "_Spawned_" + std::to_string(SpawnCount++);

		AActor* NewActor = nullptr;
		if (SpawnTypeIndex == 0) NewActor = Level->SpawnActor<ACubeActor>(Name);
		else if (SpawnTypeIndex == 1) NewActor = Level->SpawnActor<ASphereActor>(Name);
		else if (SpawnTypeIndex == 2) NewActor = Level->SpawnActor<APlaneActor>(Name);
		else if (SpawnTypeIndex == 3) NewActor = Level->SpawnActor<AAttachTestActor>(Name);
		else if (SpawnTypeIndex == 4) NewActor = Level->SpawnActor<ASubUVActor>(Name);
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
		else if (SpawnTypeIndex == 6) NewActor = Level->SpawnActor<ASkySphereActor>(Name);

		if (NewActor && !NewActor->IsA<ASkySphereActor>())
		{
			Core->SetSelectedActor(NewActor);
		}
		UE_LOG("Spawned %s: %s", SpawnTypes[SpawnTypeIndex], Name.c_str());
	}

	ImGui::SameLine();
	AActor* SelectedActor = Core->GetSelectedActor();
	if (!SelectedActor) ImGui::BeginDisabled();

	if (ImGui::Button("Delete"))
	{
		const FString Name = SelectedActor->GetName();
		Core->GetLevel()->DestroyActor(SelectedActor);
		Core->SetSelectedActor(nullptr);
		UE_LOG("Deleted actor: %s", Name.c_str());
	}

	if (!SelectedActor) ImGui::EndDisabled();

	ImGui::End();
}