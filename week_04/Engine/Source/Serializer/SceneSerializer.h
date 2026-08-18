#pragma once
#include "../Types/String.h"
#include "EngineAPI.h"
#include <d3d11.h>

class ULevel;
class UCameraComponent;

class ENGINE_API FSceneSerializer
{
public:
	// Camera는 직렬화할 ViewportClient의 ActiveCamera를 외부에서 주입
	// nullptr이면 카메라 정보 직렬화/복원 생략
	static void Save(ULevel* Level, const FString& FilePath, UCameraComponent* Camera = nullptr);
	static bool Load(ULevel* Level, const FString& FilePath, ID3D11Device* Device, UCameraComponent* Camera = nullptr);
};