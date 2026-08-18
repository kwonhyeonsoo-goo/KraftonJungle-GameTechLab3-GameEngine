#pragma once

class FCore;

class FEditorSceneManagerPanel
{
public:
	FEditorSceneManagerPanel(FCore* InCore);
	~FEditorSceneManagerPanel();

public:
	void Render();

private:
	FCore* Core = nullptr;
};