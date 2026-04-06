#include "EditorPropertyPanel.h"

#include "Core/Core.h"
#include "Scene/SceneGraph.h"

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
			FRotator Rot = SelectedPrimitiveData->GetRelativeRotation().Rotator();
			FVector Scale = SelectedPrimitiveData->GetRelativeScale();

			if (ImGui::DragFloat3("Location", &Loc.X, 0.1f))
			{
				SelectedPrimitiveData->SetRelativeLocation(Loc);
				Core->GetSceneGraph()->Build(*Core->GetScene());
				Core->GetVisibilitySystem()->BuildBVH(*Core->GetScene());
			}
			if (ImGui::DragFloat3("Rotation", &Rot.Roll, 0.1f))
			{
				SelectedPrimitiveData->SetRelativeRotation(Rot.Quaternion());
				Core->GetSceneGraph()->Build(*Core->GetScene());
				Core->GetVisibilitySystem()->BuildBVH(*Core->GetScene());
			}
			if (ImGui::DragFloat3("Scale", &Scale.X, 0.1f, 0.01f))
			{
				SelectedPrimitiveData->SetRelativeScale(Scale);
				Core->GetSceneGraph()->Build(*Core->GetScene());
				Core->GetVisibilitySystem()->BuildBVH(*Core->GetScene());
			}
		}


	}
	ImGui::End();
}