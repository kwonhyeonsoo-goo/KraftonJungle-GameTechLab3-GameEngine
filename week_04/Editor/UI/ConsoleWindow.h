#pragma once

#include <vector>
#include <string>

class CConsoleWindow
{
public:
    void Render();
    void AddLog(const char* Format, ...);
    void Clear();

private:
    std::vector<std::string> LogEntries;
};
