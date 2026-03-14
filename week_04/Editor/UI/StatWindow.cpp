#include "StatWindow.h"
#include "Object/Object.h"

void CStatWindow::Render()
{
	ImGui::Begin("Stats");

	ImGui::Text("Object Count: %d", UObject::TotalAllocationCounts);
	ImGui::Text("Heap Usage: %d bytes", UObject::TotalAllocationBytes);

	ImGuiIO& IO = ImGui::GetIO();
	ImGui::Text("FPS: %.1f (%.3f ms)", IO.Framerate, 1000.0f / IO.Framerate);

	ImGui::End();
}
