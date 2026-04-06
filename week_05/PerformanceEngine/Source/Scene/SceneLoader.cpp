#include "SceneLoader.h"
#include <windows.h>
#include <commdlg.h>

#include "Core/Core.h"
#include "Scene/Scene.h"
#include "Scene/SceneGraph.h"
#include "Graphics/D3D11/D3D11RHI.h"
#include "Camera/Camera.h"
#include "Visibility/VisibilitySystem.h"
#include "Picking/PickingSystem.h"
bool FSceneLoader::OpenSceneFileDialog(std::wstring& OutSelectedPath) const
{
	wchar_t FileName[MAX_PATH] = L"";

	OPENFILENAMEW ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = L"Scene Files (*.scene)\0*.scene\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = L"scene";

	if (GetOpenFileNameW(&ofn))
	{
		OutSelectedPath = FileName;
		return true;
	}

	return false;
}

bool FSceneLoader::LoadScene(
	const std::wstring& InScenePath,
	FCore* InCore,
	FScene* InOutScene,
	FD3D11RHI* InRHI,
	FCamera* InOutCamera,
	FVisibilitySystem* InVisibilitySystem,
	FPickingSystem* InPickingSystem)
{
	if (!InOutScene || !InRHI || !InOutCamera || !InVisibilitySystem || !InPickingSystem)
	{
		return false;
	}
	InRHI->GetDeviceContext()->ClearState();
	InOutScene->Release();
	InVisibilitySystem->Reset();
	InPickingSystem->Reset();
	InCore->GetSceneGraph()->Reset();

	if (!InOutScene->LoadFromFile(InRHI->GetDevice(), InRHI->GetDeviceContext(), InScenePath))
	{
		OutputDebugStringA("[SceneLoader] Failed to load scene.\n");
		return false;
	}

	CurrentScenePath = InScenePath;

	const FSceneCameraInitData& InitialCamera = InOutScene->GetInitialCamera();
	InOutCamera->SetTransform(InitialCamera.Transform);
	InOutCamera->SetFOV(InitialCamera.FovDegrees);
	InOutCamera->SetNearClip(InitialCamera.NearClip);
	InOutCamera->SetFarClip(InitialCamera.FarClip);

	InCore->GetSceneGraph()->Build(*InOutScene);
	InVisibilitySystem->BuildBVH(*InOutScene);

	return true;
}