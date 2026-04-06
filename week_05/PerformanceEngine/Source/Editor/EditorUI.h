#pragma once
#include <windows.h>
#include <d3d11.h>
#include <memory>

class FCore;
class FEditorControlPanel;
class FEditorConsolePanel;
class FEditorPropertyPanel;
class FEditorSceneManagerPanel;

class FEditorUI
{
public:
	FEditorUI(FCore* InCore);
	~FEditorUI();

public:
	void Initialize(HWND HWnd, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
	void Shutdown();

	void Render();

private:
	FCore* Core;

	std::unique_ptr<FEditorControlPanel> ControlPanel;
	std::unique_ptr<FEditorConsolePanel> ConsolePanel;
	std::unique_ptr<FEditorPropertyPanel> PropertyPanel;
	std::unique_ptr<FEditorSceneManagerPanel> SceneManagerPanel;
};