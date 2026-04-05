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
			FVector Loc = SelectedPrimitiveData->GetRelativeLocation();
			float Pos[3] = { Loc.X, Loc.Y, Loc.Z };
			if (ImGui::DragFloat3("Location", Pos, 0.1f))
			{
				SelectedPrimitiveData->SetRelativeLocation(FVector(Pos[0], Pos[1], Pos[2]));
			}
		}


	}
	ImGui::End();
}