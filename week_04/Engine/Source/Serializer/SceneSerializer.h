#pragma once
#include "../Types/String.h"
#include "Scene/Scene.h"
#include <d3d11.h>
class FSceneSerializer
{
public:

	static void Save(UScene* Scene, const FString& FilePath);
	static void Load(UScene* Scene, const FString& FilePath, ID3D11Device* Device);
};