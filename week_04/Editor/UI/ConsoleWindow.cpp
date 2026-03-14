#include "ConsoleWindow.h"
#include "imgui.h"
#include <cstdarg>
#include <cstdio>

void CConsoleWindow::AddLog(const char* Format, ...)
{
	char Buffer[1024];
	va_list Args;
	va_start(Args, Format);
	vsnprintf(Buffer, sizeof(Buffer), Format, Args);
	va_end(Args);
	LogEntries.emplace_back(Buffer);
	bScrollToBottom = true;
}

void CConsoleWindow::Clear()
{
	LogEntries.clear();
}

void CConsoleWindow::Render()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	bool bOpen = ImGui::Begin("Console");
	ImGui::PopStyleVar();

	if (!bOpen)
	{
		ImGui::End();
		return;
	}

	if (ImGui::Button("Clear"))
		Clear();

	ImGui::Separator();

	float ChildHeight = ImGui::GetContentRegionAvail().y;
	if (ChildHeight < 1.0f) ChildHeight = 1.0f;

	if (ImGui::BeginChild("ScrollRegion", ImVec2(0, ChildHeight), false,
		ImGuiWindowFlags_HorizontalScrollbar))
	{
		for (const auto& Entry : LogEntries)
			ImGui::TextUnformatted(Entry.c_str());

		if (bScrollToBottom)
		{
			ImGui::SetScrollHereY(1.0f);
			bScrollToBottom = false;
		}
	}
	ImGui::EndChild();
	ImGui::End();
}