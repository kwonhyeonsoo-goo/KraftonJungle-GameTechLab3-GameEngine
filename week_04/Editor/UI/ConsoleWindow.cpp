#include "ConsoleWindow.h"
#include <cstdarg>

CConsoleWindow::CConsoleWindow()
	: AutoScroll(true)
	, ScrollToBottom(false)
{
}

void CConsoleWindow::AddLog(const char* Format, ...)
{
	char Buffer[1024];
	va_list Args;
	va_start(Args, Format);
	vsnprintf(Buffer, sizeof(Buffer), Format, Args);
	Buffer[sizeof(Buffer) - 1] = 0;
	va_end(Args);
	Items.emplace_back(Buffer);
	ScrollToBottom = true;
}

void CConsoleWindow::Clear()
{
	Items.clear();
}

void CConsoleWindow::Render()
{
	ImGui::SetNextWindowSize(ImVec2(520, 300), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Console"))
	{
		ImGui::End();
		return;
	}

	// Buttons
	if (ImGui::SmallButton("Clear"))
	{
		Clear();
	}
	ImGui::SameLine();
	bool CopyToClipboard = ImGui::SmallButton("Copy");
	ImGui::SameLine();
	Filter.Draw("Filter", 180);
	ImGui::Separator();

	// Log area
	ImGuiStyle& Style = ImGui::GetStyle();
	const float FooterHeight = Style.SeparatorSize + Style.ItemSpacing.y;
	if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -FooterHeight), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar))
	{
		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Clear"))
				Clear();
			ImGui::EndPopup();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
		if (CopyToClipboard)
			ImGui::LogToClipboard();

		for (const auto& Item : Items)
		{
			const char* Str = Item.c_str();
			if (!Filter.PassFilter(Str))
				continue;

			// Color coding
			ImVec4 Color;
			bool HasColor = false;
			if (strstr(Str, "[error]"))
			{
				Color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
				HasColor = true;
			}
			else if (strstr(Str, "[warn]"))
			{
				Color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
				HasColor = true;
			}

			if (HasColor)
				ImGui::PushStyleColor(ImGuiCol_Text, Color);
			ImGui::TextUnformatted(Str);
			if (HasColor)
				ImGui::PopStyleColor();
		}

		if (CopyToClipboard)
			ImGui::LogFinish();

		if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
			ImGui::SetScrollHereY(1.0f);
		ScrollToBottom = false;

		ImGui::PopStyleVar();
	}
	ImGui::EndChild();

	ImGui::End();
}
