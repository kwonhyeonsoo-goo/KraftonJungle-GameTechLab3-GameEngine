#pragma once

class FCore;

class FEditorPropertyPanel
{
public:
	FEditorPropertyPanel(FCore* InCore);
	~FEditorPropertyPanel();

public:
	void Render();

private:
	FCore* Core = nullptr;
};