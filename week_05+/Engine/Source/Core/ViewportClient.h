#pragma once

#include "EngineAPI.h"
#include "Windows.h"
#include "Types/String.h"
#include "ShowFlags.h"
#include "Renderer/RenderCommand.h"
#include "World/RenderCollector.h"
#include "World/LevelTypes.h"
#include "Input/InputManager.h"
#include "Camera/Camera.h"
#include "Input/InputAction.h"
#include "Input/EnhancedInputManager.h"
#include "Math/Rect.h"


class FCore;
class FRenderer;
class ULevel;
class FFrustum;
class UPrimitiveComponent;
struct FRenderCommandQueue;
class UWorld;
struct FInputMappingContext;
class FViewport;
class SViewportWindow;

class ENGINE_API FViewportClient
{
public:
	FViewportClient() = default;
	virtual ~FViewportClient() = default;

	virtual void Attach(FCore* Core);
	virtual void Detach();
	virtual void HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);
	virtual ULevel* ResolveLevel(FCore* Core) const;
	virtual UWorld* ResolveWorld(FCore* Core) const;
	virtual const char* GetViewportLabel() const { return "Viewport"; }
	virtual FMatrix GetViewMatrix() const;
	virtual FMatrix GetProjectionMatrix(float AspectRatio) const;
	virtual FMatrix GetViewProjectionMatrix(float AspectRatio) const;
	FShowFlags& GetShowFlags() { return ShowFlags; }
	const FShowFlags& GetShowFlags() const { return ShowFlags; }
	virtual void BuildRenderCommands(TArray<AActor*>& InActors, FRenderCommandQueue& OutQueue);
	virtual void PostRender(FCore* Core, FRenderer* Renderer);
	FCamera* GetCamera() { return &CameraTransform; }
	virtual void DrawUI();
	void SetViewportWindow(SViewportWindow* InViewportWindow);


	virtual void Initialize(FInputManager* InInput, FEnhancedInputManager* InEnhancedInput);
	virtual void SetupInputBindings();
	virtual void ProcessCameraInput(FCore* Core, float DeltaTime);
	virtual void Cleanup();
	virtual void Tick(float DeltaTime);
	virtual void SetViewportRect(const FRect& InRect);
	virtual void SetViewportInputState(int32 InMouseX, int32 InMouseY, const FRect& InRect);
	void SetWorldType(ELevelType InWorldType);
	ELevelType GetWorldType() const;
	int32 GetViewportWidth() const { return ViewportWidth; }
	int32 GetViewportHeight() const { return ViewportHeight; }

protected:
	FCamera CameraTransform;
	FShowFlags ShowFlags;
	FLevelRenderCollector RenderCollector;
	FInputManager* InputManager = nullptr;
	FEnhancedInputManager* EnhancedInput = nullptr;
	FInputMappingContext* CameraContext = nullptr;

	FInputAction MoveForwardAction{ "MoveForward", EInputActionValueType::Float };
	FInputAction MoveRightAction{ "MoveRight", EInputActionValueType::Float };
	FInputAction MoveUpAction{ "MoveUp", EInputActionValueType::Float };
	FInputAction LookXAction{ "LookX", EInputActionValueType::Float };
	FInputAction LookYAction{ "LookY", EInputActionValueType::Float };

	float CurrentDeltaTime = 0.0f;
	int32 ViewportTopLeftX = 0;
	int32 ViewportTopLeftY = 0;
	bool bIsViewportHovered = false;
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	int32 ViewportMouseX = 0;
	int32 ViewportMouseY = 0;
	ELevelType WorldType = ELevelType::Game;
	SViewportWindow* ViewportWindow = nullptr;

};

class ENGINE_API FGameViewportClient : public FViewportClient
{
public:
	void Attach(FCore* Core) override;
	void Detach() override;
};
