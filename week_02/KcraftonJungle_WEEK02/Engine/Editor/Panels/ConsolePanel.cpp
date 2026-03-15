#include "ConsolePanel.h"

void ConsolePanel::OnRender(AppContext& ctx)
{
    ImGui::Begin("Console");

    ImGui::BeginChild("ScrollRegion", ImVec2(0, -30));
    const TArray<FString>& logs = ctx.Console.GetLogs();
    for (int i = 0; i < logs.size(); i++)
        ImGui::TextUnformatted(logs[i].c_str());

    if (ScrollToBottom)
        ImGui::SetScrollHereY(1.0f);
    ScrollToBottom = false;

    ImGui::EndChild();

    if (ImGui::InputText("##input", InputBuf, sizeof(InputBuf),
        ImGuiInputTextFlags_EnterReturnsTrue))
    {
        ctx.Console.AddLog(FString(InputBuf));
        InputBuf[0] = '\0';
        ScrollToBottom = true;
    }

    ImGui::End();
}