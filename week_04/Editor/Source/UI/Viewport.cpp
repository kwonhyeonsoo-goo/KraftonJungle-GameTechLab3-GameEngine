#include "Viewport.h"

#include "imgui.h"
#include "imgui_internal.h"

void CViewport::Render(CCore* Core)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	bool bOpen = ImGui::Begin("Viewport");
	ImGui::PopStyleVar();

	if (!bOpen)
	{
		ImGui::End();
		return;
	}

	if (ImGui::IsWindowFocused(ImGuiFocusRequestFlags_RestoreFocusedChild))
	{
		// TODO : 키보드 마우스 input 로직 넣기
	}

	const ImVec2 NewSize = ImGui::GetContentRegionAvail();

	
}

void CViewport::ReadySceneView(uint32 Width, uint32 Height)
{
	if (Width <= 0 || Height <= 0)
	{
		RenderTargetView->Release();

		return;
	}
}
