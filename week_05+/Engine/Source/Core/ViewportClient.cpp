#include "ViewportClient.h"
#include "World/World.h"
#include "Core/Core.h"
#include "Camera/Camera.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/Material.h"
#include "World/Level.h"
#include "Debug/EngineLog.h"
#include "Component/UUIDBillboardComponent.h"
#include "Component/SubUVComponent.h"
#include "Core/FEngine.h"
#include "Component/TextComponent.h"
#include "Input/InputMappingContext.h"

void FViewportClient::Attach(FCore* Core)
{
}

void FViewportClient::Detach()
{
}

void FViewportClient::Initialize(FInputManager* InInput, FEnhancedInputManager* InEnhancedInput)
{
	InputManager = InInput;
	EnhancedInput = InEnhancedInput;
	SetupInputBindings();
}

void FViewportClient::Cleanup()
{
	if (EnhancedInput && CameraContext)
	{
		EnhancedInput->RemoveMappingContext(CameraContext);
	}
	delete CameraContext;
	CameraContext = nullptr;
	EnhancedInput = nullptr;
}

void FViewportClient::Tick(float DeltaTime)
{
	CurrentDeltaTime = DeltaTime;
}

void FViewportClient::ProcessCameraInput(FCore* Core, float DeltaTime)
{
	(void)Core;
	(void)DeltaTime;
}

void FViewportClient::SetViewportRect(const FRect& InRect)
{
	ViewportTopLeftX = static_cast<int32>(InRect.Position.X);
	ViewportTopLeftY = static_cast<int32>(InRect.Position.Y);
	ViewportWidth = static_cast<int32>(InRect.Size.X);
	ViewportHeight = static_cast<int32>(InRect.Size.Y);

	if (ViewportWidth > 0 && ViewportHeight > 0)
	{
		CameraTransform.SetAspectRatio(static_cast<float>(ViewportWidth) / static_cast<float>(ViewportHeight));
	}
}

void FViewportClient::SetViewportInputState(int32 InMouseX, int32 InMouseY, const FRect& InRect)
{
	ViewportMouseX = InMouseX;
	ViewportMouseY = InMouseY;
	SetViewportRect(InRect);
}

void FViewportClient::SetWorldType(ELevelType InWorldType)
{
	WorldType = InWorldType;
}

ELevelType FViewportClient::GetWorldType() const
{
	return WorldType;
}

void FViewportClient::SetupInputBindings()
{
}

void FViewportClient::HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
}

ULevel* FViewportClient::ResolveLevel(FCore* Core) const
{
	UWorld* World = ResolveWorld(Core);
	return World ? World->GetLevel() : nullptr;
}

UWorld* FViewportClient::ResolveWorld(FCore* Core) const
{
	if (!Core)
	{
		return nullptr;
	}

	switch (WorldType)
	{
	case ELevelType::Editor:
		return Core->GetEditorWorld();
	case ELevelType::Game:
	case ELevelType::PIE:
		return Core->GetGameWorld();
	case ELevelType::Inactive:
	default:
		return Core->GetActiveWorld();
	}
}

FMatrix FViewportClient::GetViewMatrix() const
{
	return CameraTransform.GetViewMatrix();
}

FMatrix FViewportClient::GetProjectionMatrix(float AspectRatio) const
{
	(void)AspectRatio;
	return CameraTransform.GetProjectionMatrix();
}

FMatrix FViewportClient::GetViewProjectionMatrix(float AspectRatio) const
{
	return GetViewMatrix() * GetProjectionMatrix(AspectRatio);
}

void FViewportClient::BuildRenderCommands(TArray<AActor*>& InActors, FRenderCommandQueue& OutQueue)
{
	FFrustum Frustum;
	const float AspectRatio = ViewportHeight > 0 ? static_cast<float>(ViewportWidth) / static_cast<float>(ViewportHeight) : 1.0f;
	const FMatrix ViewMatrix = GetViewMatrix();
	const FMatrix ProjectionMatrix = GetProjectionMatrix(AspectRatio);
	const FMatrix ViewProjection = ViewMatrix * ProjectionMatrix;
	Frustum.ExtractFromVP(ViewProjection);

	OutQueue.ViewMatrix = ViewMatrix;
	OutQueue.ProjectionMatrix = ProjectionMatrix;
	RenderCollector.CollectRenderCommands(InActors, Frustum, ShowFlags, &CameraTransform, OutQueue);
}

void FViewportClient::PostRender(FCore* Core, FRenderer* Renderer)
{
	(void)Core;
	(void)Renderer;
}

void FViewportClient::DrawUI()
{
}

void FViewportClient::SetViewportWindow(SViewportWindow* InViewportWindow)
{
	ViewportWindow = InViewportWindow;
}

void FGameViewportClient::Attach(FCore* Core)
{
	(void)Core;
	if (GRenderer)
	{
		GRenderer->ClearViewportCallbacks();
	}
}

void FGameViewportClient::Detach()
{
	if (GRenderer)
	{
		GRenderer->ClearViewportCallbacks();
	}
}
