#include "StatWindow.h"
#include "imgui.h"

void CStatWindow::Render()
{

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	ImGui::PopStyleVar();
	if (!ImGui::Begin("Stats"))
	{
		ImGui::End();
		return;
	}

	ImGuiIO& IO = ImGui::GetIO();
	ImGui::Text("FPS        : %.1f  (%.3f ms)", IO.Framerate, 1000.0f / IO.Framerate);
	ImGui::Text("Objects    : %u", ObjectCount);
	ImGui::Text("Heap Usage : %.2f KB", HeapUsageBytes / 1024.0f);

	ImGui::End();
}