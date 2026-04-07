#include "EditorToolbar.h"

#include "FEditorEngine.h"

namespace
{
	void StartPIE()
	{
		UWorld* EditorWorld = GEditor->GetEditorWorldContext().World;

		UWorld* PIEWorld = UWorld::DuplicateWorldForPIE(EditorWorld);
		GWorld = PIEWorld;

		FEngine::CreateWorldContext(EWorldType::PIE, GWorld);

		PIEWorld->BeginPlay();
	}

	void EndPIE()
	{
		if (GWorld && GWorld->GetWorldType() == EWorldType::PIE)
		{
			GWorld->CleanupWorld();
			GEditor->RemoveEditorWorldContext(EWorldType::PIE);
			delete GWorld;
		}

		GWorld = GEditor->GetEditorWorldContext().World;
	}
}


void FEditorToolbar::Render()
{
	EPIEState PIEState = GEditor->GetPIEState();
	if (PIEState == EPIEState::Stopped)
	{
		if (ImGui::Button("▶ Play"))
		{
			GEditor->SetPIEState(EPIEState::Playing);
			StartPIE();
		}
	}
	else if (PIEState == EPIEState::Playing)
	{
		if (ImGui::Button("Ⅱ Pause"))
		{
			GEditor->SetPIEState(EPIEState::Paused);
		}
		ImGui::SameLine();
		if (ImGui::Button("■ Stop"))
		{
			GEditor->SetPIEState(EPIEState::Stopped);
			EndPIE();
		}
	}
	else if (PIEState == EPIEState::Paused)
	{
		if (ImGui::Button("▶ Resume"))
		{
			GEditor->SetPIEState(EPIEState::Playing);
		}
		ImGui::SameLine();
		if (ImGui::Button("■ Stop"))
		{
			GEditor->SetPIEState(EPIEState::Stopped);
			EndPIE();
		}
	}
}