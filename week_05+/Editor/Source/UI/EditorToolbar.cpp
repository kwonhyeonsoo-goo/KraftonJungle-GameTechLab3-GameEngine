#include "EditorToolbar.h"
#include "FEditorEngine.h"
#include "Core/GameViewportClient.h"

namespace
{
	void StartPIE()
	{
		UWorld* EditorWorld = GEditor->GetEditorWorldContext().World;

		UWorld* PIEWorld = UWorld::DuplicateWorldForPIE(EditorWorld);
		GWorld = PIEWorld;

		FEngine::CreateWorldContext(EWorldType::PIE, GWorld);

		GEditor->ChangeGameViewportClient();
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
		GEditor->RestoreEditorViewportClient();
	}
}

void FEditorToolbar::Render()
{
	// 일반 도킹 가능한 윈도우 스타일 플래그 (스크롤바만 제거)
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	bool bOpen = ImGui::Begin("Toolbar", nullptr, windowFlags);
	
	if (!bOpen)
	{
		ImGui::End();
		return;
	}

	EPIEState PIEState = GEditor->GetPIEState();

	// 버튼 크기 및 둥근 모서리 설정
	ImVec2 buttonSize(80, 28);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

	// 3개의 버튼과 여백을 고려한 전체 폭
	float spacing = ImGui::GetStyle().ItemSpacing.x;
	float totalWidth = (buttonSize.x * 3) + (spacing * 2);

	// 중앙 정렬 (가로)
	float availWidth = ImGui::GetContentRegionAvail().x;
	ImGui::SetCursorPosX(max(0.0f, (availWidth - totalWidth) * 0.5f));

	// 중앙 정렬 (세로) - 도킹 패널 안에서 세로 중앙을 맞추기 위함
	float availHeight = ImGui::GetContentRegionAvail().y;
	if (availHeight > buttonSize.y)
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (availHeight - buttonSize.y) * 0.5f);
	}

	// -------------------------------------------------------------
	// 1. Play / Resume 버튼
	// -------------------------------------------------------------
	bool bCanPlay = (PIEState == EPIEState::Stopped || PIEState == EPIEState::Paused);
	const char* playText = (PIEState == EPIEState::Paused) ? "▶ Resume" : "▶ Play";

	ImGui::BeginDisabled(!bCanPlay);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.5f, 0.15f, 1.0f));

	if (ImGui::Button(playText, buttonSize))
	{
		if (PIEState == EPIEState::Stopped)
		{
			StartPIE();
		}
		GEditor->SetPIEState(EPIEState::Playing);
	}
	ImGui::PopStyleColor(3);
	ImGui::EndDisabled();

	ImGui::SameLine();

	// -------------------------------------------------------------
	// 2. Pause 버튼
	// -------------------------------------------------------------
	bool bCanPause = (PIEState == EPIEState::Playing);

	ImGui::BeginDisabled(!bCanPause);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.75f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.4f, 0.0f, 1.0f));

	if (ImGui::Button("Ⅱ", buttonSize))
	{
		GEditor->SetPIEState(EPIEState::Paused);
	}
	ImGui::PopStyleColor(3);
	ImGui::EndDisabled();

	ImGui::SameLine();

	// -------------------------------------------------------------
	// 3. Stop 버튼
	// -------------------------------------------------------------
	bool bCanStop = (PIEState != EPIEState::Stopped);

	ImGui::BeginDisabled(!bCanStop);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.15f, 0.15f, 1.0f));

	if (ImGui::Button("■", buttonSize))
	{
		GEditor->SetPIEState(EPIEState::Stopped);
		EndPIE();
	}
	ImGui::PopStyleColor(3);
	ImGui::EndDisabled();

	ImGui::PopStyleVar();
	ImGui::End();
}