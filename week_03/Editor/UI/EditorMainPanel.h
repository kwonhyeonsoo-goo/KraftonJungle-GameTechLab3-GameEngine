#pragma once

#include <windows.h>

#include "Math/Vector.h"
#include "Editor/Core/EditorConsole.h"
#include "Core/Common.h"
#include "Object/Object.h"

class FRenderer;
class FEditorEngine;
class FEditorViewportClient;

enum class EPrimitiveType;

using namespace common::structs;

class FEditorMainPanel
{
private:
	const char* PrimitiveTypes[3] =
	{
		"Cube",
		"Sphere",
		"Plane"
	};
	FEditorEngine* EditorEngine = nullptr;
	FRenderer* Renderer;
	FEditorViewportClient* Viewport;

	int SelectedPrimitiveType = 0;
	int NumberOfSpawnedActors = 1;
	FVector CurSpawnPoint = { 0.f,0.f,0.f };
	bool bShowConsole = true;

	// Scene stuffs
	float NewSceneNotificationTimer = 0;
	float SceneSaveNotificationTimer = 0;
	float SceneLoadNotificationTimer = 0;
	uint32 SelectedSceneIndex = -1;

	// Scene Manager stuffs
	uint32 SelectedActorIndex = 0;

	FEditorConsole ConsoleInstance;
	
public:
	void Create(HWND InHWindow, FRenderer& InRenderer, FEditorEngine* InEditorEngine, FEditorViewportClient* InEditorViewport);
	void Release();
	void Render(float DeltaTime, FViewOutput& ViewOutput);
	void RenderObjectManager(FViewOutput& ViewOutput);
	void RenderObjectWindow(UObject*& Object);
	void Update();
};
