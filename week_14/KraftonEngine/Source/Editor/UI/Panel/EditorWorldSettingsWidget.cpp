#include "Editor/UI/Panel/EditorWorldSettingsWidget.h"
#include "Engine/Runtime/Engine.h"
#include "GameFramework/World.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/GameMode/GameModeBase.h"
#include "Object/Reflection/UClass.h"
#include "ImGui/imgui.h"

#include <cstring>

void EditorWorldSettingsWidget::Render()
{
	if (!bOpen) return;

	ImGui::SetNextWindowSize(ImVec2(360, 300), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("World Settings", &bOpen))
	{
		ImGui::End();
		return;
	}

	UWorld* World = GEngine ? GEngine->GetWorld() : nullptr;
	if (!World)
	{
		ImGui::TextDisabled("No active world.");
		ImGui::End();
		return;
	}

	FWorldSettings& WS = World->GetWorldSettings();

	if (ImGui::CollapsingHeader("Game", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// GameMode 클래스 — UClass 레지스트리에서 AGameModeBase 파생만 필터링.
		// 첫 항목 "(Default)" = 빈 문자열 → ProjectSettings fallback.
		TArray<UClass*> GameModeClasses;
		GameModeClasses.push_back(nullptr); // sentinel for "(Default)"
		for (UClass* C : UClass::GetAllClasses())
		{
			if (C && C->IsA(AGameModeBase::StaticClass()))
				GameModeClasses.push_back(C);
		}

		int GMIdx = 0;
		for (int i = 1; i < static_cast<int>(GameModeClasses.size()); ++i)
		{
			if (WS.GameModeClassName == GameModeClasses[i]->GetName())
			{
				GMIdx = i;
				break;
			}
		}

		const char* GMPreview = (GMIdx == 0) ? "(Default)" : GameModeClasses[GMIdx]->GetName();
		if (ImGui::BeginCombo("GameMode Class", GMPreview))
		{
			for (int i = 0; i < static_cast<int>(GameModeClasses.size()); ++i)
			{
				const char* Label = (i == 0) ? "(Default)" : GameModeClasses[i]->GetName();
				bool bSelected = (i == GMIdx);
				if (ImGui::Selectable(Label, bSelected))
				{
					WS.GameModeClassName = (i == 0) ? FString() : FString(GameModeClasses[i]->GetName());
				}
				if (bSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		char DefaultPawnPrefabPath[512] = {};
		strncpy_s(DefaultPawnPrefabPath, WS.DefaultPawnPrefabPath.c_str(), _TRUNCATE);
		if (ImGui::InputText("Default Pawn Prefab", DefaultPawnPrefabPath, sizeof(DefaultPawnPrefabPath)))
		{
			WS.DefaultPawnPrefabPath = DefaultPawnPrefabPath;
		}
		ImGui::TextDisabled("Save scene + reload to apply.");
	}

	if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat3("Gravity", WS.Gravity.Data, 0.01f, -100.0f, 100.0f, "%.2f");
		ImGui::TextDisabled("m/s^2, saved with the current scene.");
	}

	if (ImGui::CollapsingHeader("Ballistics", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Enable Ballistic Wind", &WS.bEnableBallisticWind);
		ImGui::BeginDisabled(!WS.bEnableBallisticWind);
		ImGui::DragFloat3("Wind Acceleration", WS.BallisticWindAcceleration.Data, 0.01f, -100.0f, 100.0f, "%.2f");
		ImGui::EndDisabled();
		ImGui::TextDisabled("Scene-wide wind shared by all sniper bullets.");
	}

	ImGui::End();
}
