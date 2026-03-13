#include "ConsoleWindow.h"
// #include <imgui.h>
#include <cstdarg>

void CConsoleWindow::Render()
{
    // TODO: ImGui 콘솔 창 — ShowExampleAppConsole() 참고
}

void CConsoleWindow::AddLog(const char* Format, ...)
{
    char Buffer[1024];
    va_list Args;
    va_start(Args, Format);
    vsnprintf(Buffer, sizeof(Buffer), Format, Args);
    va_end(Args);
    LogEntries.emplace_back(Buffer);
}

void CConsoleWindow::Clear()
{
    LogEntries.clear();
}
