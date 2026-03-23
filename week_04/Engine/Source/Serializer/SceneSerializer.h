#pragma once
#include "../Types/String.h"

class UScene;
class FSceneSerializer
{
public:

	static void Save(UScene* Scene, const FString& FilePath);
	static void Load(UScene* Scene, const FString& FilePath, ID3D11Device* Device);
};