#include "EditorSceneManagerPanel.h"

#include "Core/Core.h"
#include "Scene/Scene.h"

#include "Thirdparty/ImGui/imgui.h"

FEditorSceneManagerPanel::FEditorSceneManagerPanel(FCore* InCore)
{
	Core = InCore;
}

FEditorSceneManagerPanel::~FEditorSceneManagerPanel()
{
}

void FEditorSceneManagerPanel::Render()
{
	if (ImGui::Begin("Scene Manager"))
	{
		FScene* Scene = Core->GetScene();
		TArray<FScenePrimitiveRuntimeData> RuntimeData = Scene->GetPrimitiveRuntimeData();

		if (ImGui::TreeNodeEx("Primitives", ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (const FScenePrimitiveRuntimeData& Primitive : RuntimeData)
			{
				ImGui::PushID(Primitive.PrimitiveId);

				ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

				FPickState& PickState = Core->GetPickState();
				if (PickState.bHit && PickState.SelectedPrimitiveId == Primitive.PrimitiveId)
				{
					NodeFlags |= ImGuiTreeNodeFlags_Selected;
				}

				bool isOpen = ImGui::TreeNodeEx((void*)(intptr_t)Primitive.PrimitiveId, NodeFlags, "Primitive ID: %d", Primitive.PrimitiveId);

				if (ImGui::IsItemClicked())
				{
					PickState.SelectedPrimitiveId = Primitive.PrimitiveId;
					PickState.SelectedPrimitiveIndex = Primitive.PrimitiveId; // TODO: PrimitiveId와 Index가 항상 일치하는 것은 아니므로, 실제로는 Scene에서 PrimitiveId로 Index를 찾아야 함
					PickState.bHit = true;
				}

				ImGui::PopID();
			}
			ImGui::TreePop();
		}
	}

	ImGui::End();
}