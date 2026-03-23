#include "OutlinerWindow.h"

#include "imgui.h"
#include "Core/Core.h"
#include "Core/ShowFlags.h"
#include "Core/ViewportClient.h"
#include "Scene/Scene.h"
#include "Actor/Actor.h"
#include "Component/SubUVComponent.h"

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

	IViewportClient* ViewportCli = Core->GetViewportClient();
	if (!ViewportCli) { ImGui::End(); return; }

	FShowFlags & ShowFlags = ViewportCli->GetShowFlags();
	AActor* SelectedActor = Core->GetSelectedActor();


	// ===== Show Flags 섹션 =====
	ImGui::SeparatorText("Show Flags");
	// 각 플래그마다 Checkbox 하나씩
	bool bPrimitives = ShowFlags.HasFlag(EEngineShowFlags::SF_Primitives);
	if (ImGui::Checkbox("Primitives", &bPrimitives))
	{
		ShowFlags.SetFlag(EEngineShowFlags::SF_Primitives, bPrimitives);
	}
	bool bUUID = ShowFlags.HasFlag(EEngineShowFlags::SF_UUID);
	if (ImGui::Checkbox("UUID", &bUUID))
	{
		ShowFlags.SetFlag(EEngineShowFlags::SF_UUID, bUUID);
	}


	if (SelectedActor)
	{
		for (UActorComponent* Component : SelectedActor->GetComponents())
		{
			if (!Component || !Component->IsA(USubUVComponent::StaticClass()))
			{
				continue;
			}

			USubUVComponent* SubUVComponent = static_cast<USubUVComponent*>(Component);
			bool bBillboard = SubUVComponent->IsBillboard();

			if (ImGui::Checkbox("SubUV Billboard", &bBillboard))
			{
				SubUVComponent->SetBillboard(bBillboard);
			}

			break;
		}
	}

	ImGui::SeparatorText("Actors");

	UScene* Scene = Core->GetScene();
	const TArray<AActor*>& Actors = Scene->GetActors();
	

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
