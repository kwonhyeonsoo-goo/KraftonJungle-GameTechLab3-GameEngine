#pragma once

#include "CoreMinimal.h"
#include "Windows.h"
#include "Core/FTimer.h"
#include "World/WorldType.h"
#include "Renderer/Renderer.h"
#include "World/WorldContext.h"
#include <memory>
#include "Debug/DebugDrawManager.h"

class FEnhancedInputManager;
class FInputManager;

class AActor;
class ULevel;
class ObjectManager;
class FViewportClient;
struct FViewportContext;

class ENGINE_API FCore
{
public:
	enum class EStatOverlayMode : uint8
	{
		None = 0,
		FPS = 1 << 0,
		Memory = 1 << 1
	};

	FCore() = default;
	~FCore();

	FCore(const FCore&) = delete;
	FCore(FCore&&) = delete;
	FCore& operator=(const FCore&) = delete;
	FCore& operator=(FCore&&) = delete;

	bool Initialize(HWND Hwnd, int32 Width, int32 Height, EWorldType StartupLevelType = EWorldType::Game);
	void Release();

	void ProcessInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);

	void OnResize(int32 Width, int32 Height);

	uint8 GetStatOverlayMode() const { return StatOverlayModeFlags; }
	void SetStatOverlayMode(uint8 InFlags) { StatOverlayModeFlags = InFlags; }
	void RenderStatOverlay(FRenderer* Renderer, int32 ViewportWidth, int32 ViewportHeight) const;

	FDebugDrawManager& GetDebugDrawManager() { return DebugDrawManager; }

private:
	void RegisterConsoleVariables();

private:
	FDebugDrawManager DebugDrawManager;

	int32 WindowWidth = 0;
	int32 WindowHeight = 0;
	uint8 StatOverlayModeFlags = static_cast<uint8>(EStatOverlayMode::None);
};
