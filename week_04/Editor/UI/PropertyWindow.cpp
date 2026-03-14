#include "PropertyWindow.h"
#include "Object/Actor/Actor.h"
#include "Component/SceneComponent.h"

void CPropertyWindow::Render(AActor* SelectedActor)
{
	ImGui::Begin("Property");

	if (SelectedActor)
	{
		ImGui::Text("Selected: %s", SelectedActor->GetName().c_str());

		USceneComponent* Root = SelectedActor->GetRootComponent();
		if (Root)
		{
			FTransform Transform = Root->GetRelativeTransform();

			float Loc[3] = { Transform.Location.X, Transform.Location.Y, Transform.Location.Z };
			float Rot[3] = { Transform.Rotation.X, Transform.Rotation.Y, Transform.Rotation.Z };
			float Scl[3] = { Transform.Scale.X, Transform.Scale.Y, Transform.Scale.Z };

			if (ImGui::DragFloat3("Location", Loc, 0.1f))
			{
				Transform.Location = { Loc[0], Loc[1], Loc[2] };
				Root->SetRelativeTransform(Transform);
			}
			if (ImGui::DragFloat3("Rotation", Rot, 0.1f))
			{
				Transform.Rotation = { Rot[0], Rot[1], Rot[2] };
				Root->SetRelativeTransform(Transform);
			}
			if (ImGui::DragFloat3("Scale", Scl, 0.1f))
			{
				Transform.Scale = { Scl[0], Scl[1], Scl[2] };
				Root->SetRelativeTransform(Transform);
			}
		}
	}
	else
	{
		ImGui::Text("No actor selected");
	}

	ImGui::End();
}
