#pragma once
#include <string>
#include <vector>

class CCore;

class CControlPanelWindow
{
public:
    void Render(CCore* Core);

private:
    std::vector<std::string> SceneFiles;
    int SelectedSceneIndex = -1;

}; 