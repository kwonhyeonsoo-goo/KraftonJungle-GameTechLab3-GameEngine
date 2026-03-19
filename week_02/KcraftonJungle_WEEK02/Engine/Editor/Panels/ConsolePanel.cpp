#include "ConsolePanel.h"
#include "../../../AppContext.h"
#include <iostream>
#include "../../Foundation/Core/log.h"

void ConsolePanel::OnRender(AppContext& ctx)
{

    float w = (float)ctx.Window.GetWidth();
    float h = (float)ctx.Window.GetHeight();
    float consoleHeight = h * 0.2f;

    // 하단 width 100%
    ImGui::SetNextWindowPos(ImVec2(0, h - consoleHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, consoleHeight), ImGuiCond_Always);

    const TArray<FString>& logs = ctx.Console.GetLogs();

    for (int i = LastCount; i < logs.size(); i++)
    {
        std::cout << logs[i].c_str() << std::endl;
        Log.AddLog("%s\n", logs[i].c_str());
    }
    LastCount = logs.size();

    if (ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
        if (ImGui::Button("Clear All")) {
            ctx.Console.Clear();
            Log.Clear();
            LastCount = 0;
        }
        float footerHeight = ImGui::GetStyle().ItemSpacing.y
            + ImGui::GetFrameHeightWithSpacing();

        ImGui::BeginChild("ScrollingRegion",
            ImVec2(0, -footerHeight), false);

        Log.Draw("Console");

        if (ScrollToBottom)
            ImGui::SetScrollHereY(1.0f);
        ScrollToBottom = false;
    }
    ImGui::Separator();
    ImGui::EndChild();

    bool reclaimFocus = false;
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (ImGui::InputText("Input", InputBuf, sizeof(InputBuf), flags)) {
        if (InputBuf[0] != '\0') {
            // 입력한 내용을 로그에 추가
            UE_LOG("> %s", InputBuf);
            ScrollToBottom = true;
        }
        memset(InputBuf, 0, sizeof(InputBuf)); // 입력창 비우기
        reclaimFocus = true;
    }

    // Enter 후 입력창에 포커스 유지
    ImGui::SetItemDefaultFocus();
    if (reclaimFocus)
        ImGui::SetKeyboardFocusHere(-1);
    ImGui::End();
}