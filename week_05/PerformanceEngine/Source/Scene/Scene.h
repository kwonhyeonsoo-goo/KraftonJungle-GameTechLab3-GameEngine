#pragma once

#include <filesystem>

#include "Graphics/D3D11/D3D11Common.h"
#include "Scene/SceneTypes.h"
#include "StaticMesh/StaticMeshManager.h"
#include "Types/Array.h"

class FScene
{
public:
	bool LoadFromFile(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, const std::filesystem::path& InSceneFilePath);
	void Release();

	const TArray<FRenderItem>& GetRenderItems() const { return RenderItems; }
	const FSceneCameraInitData& GetInitialCamera() const { return InitialCamera; }

	const FVector& GetSceneBoundsMin() const { return SceneBoundsMin; }
	const FVector& GetSceneBoundsMax() const { return SceneBoundsMax; }

private:
	TArray<FRenderItem> RenderItems;
	FStaticMeshManager MeshManager;

	FSceneCameraInitData InitialCamera;

	FVector RawCameraLocation = FVector::ZeroVector;
	FVector RawCameraRotation = FVector::ZeroVector;

	FVector SceneBoundsMin = FVector::ZeroVector;
	FVector SceneBoundsMax = FVector::ZeroVector;
};
