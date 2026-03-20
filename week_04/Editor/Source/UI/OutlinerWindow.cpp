#include "OutlinerWindow.h"

#include "imgui.h"
#include "Core/Core.h"
#include "Scene/Scene.h"

#include "Actor/Actor.h"

void COutlinerWindow::Render(CCore* Core)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	const bool bOpen = ImGui::Begin("Outliner");
	ImGui::PopStyleVar();
	if (!bOpen)
	{
		ImGui::End();
		return;
	}
	if (!Core || !Core->GetScene())
	{
		ImGui::End();
		return;
	}

	UScene* Scene = Core->GetScene();
	ImGui::SeparatorText("Actors");

	const TArray<AActor*>& Actors = Scene->GetActors();
	AActor* SelectedActor = Core->GetSelectedActor();

	for (AActor* Actor : Actors)
	{
		if (!Actor || Actor->IsPendingDestroy())
		{
			continue;
		}

		const bool bSelected = (Actor == SelectedActor);

		if (ImGui::Selectable(Actor->GetName().c_str(), bSelected))
		{
			Core->SetSelectedActor(Actor);
		}
	}

	ImGui::End();

}
