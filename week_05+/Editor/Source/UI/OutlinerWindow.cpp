#include "OutlinerWindow.h"

#include "imgui.h"
#include "Core/Core.h"
#include "Core/ShowFlags.h"
#include "Core/ViewportClient.h"
#include "World/Level.h"
#include "Actor/Actor.h"
#include "Component/SubUVComponent.h"
#include "Component/TextComponent.h"
#include "Component/UUIDBillboardComponent.h"

void FOutlinerWindow::Render(FCore* Core)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	const bool bOpen = ImGui::Begin("Outliner");
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


	AActor* SelectedActor = Core->GetSelectedActor();

	ImGui::SeparatorText("Actors");

	ULevel* Level = Core->GetLevel();
	const TArray<AActor*>& Actors = Level->GetActors();
	

	for (AActor* Actor : Actors)
	{
;
		if (!Actor || Actor->IsPendingDestroy())
		{
			continue;
		}

		const bool bSelected = (Actor == SelectedActor);
		ImGui::PushID(Actor);
		bool bVisible = Actor->IsVisible();
		if (ImGui::Checkbox("##visible", &bVisible))
		{
			Actor->SetVisible(bVisible);
		}
		ImGui::SameLine();

		if (ImGui::Selectable(Actor->GetName().c_str(), bSelected))
		{
			Core->SetSelectedActor(Actor);
		}
		ImGui::PopID();
	}

	ImGui::End();

}
