#include "EditorPropertyPanel.h"

#include "Core/Core.h"

#include "Thirdparty/ImGui/imgui.h"

FEditorPropertyPanel::FEditorPropertyPanel(FCore* InCore)
{
	Core = InCore;
}

FEditorPropertyPanel::~FEditorPropertyPanel()
{
}

void FEditorPropertyPanel::Render()
{
	if (ImGui::Begin("Property Panel"))
	{
		FScenePrimitiveRuntimeData* SelectedPrimitiveData = const_cast<FScenePrimitiveRuntimeData*>(Core->GetSelectedPrimitiveData());
		if (SelectedPrimitiveData)
		{
			ImGui::DragFloat3("Location", &SelectedPrimitiveData->WorldMatrix.M[3][0], 0.1f);
		}


	}
	ImGui::End();
}