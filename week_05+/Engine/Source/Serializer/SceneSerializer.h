#pragma once
#include "../Types/String.h"
#include "EngineAPI.h"
#include <d3d11.h>
class ULevel;
class FCamera;
class ENGINE_API FSceneSerializer
{
public:

	static void Save(ULevel* Level, const FString& FilePath, const FCamera* PerspectiveCamera = nullptr);
	static bool Load(ULevel* Level, const FString& FilePath, ID3D11Device* Device, FCamera* PerspectiveCamera = nullptr);
};
