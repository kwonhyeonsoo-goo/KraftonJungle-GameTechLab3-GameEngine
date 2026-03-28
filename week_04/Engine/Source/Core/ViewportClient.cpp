#include "ViewportClient.h"
#include "World/World.h"
#include "Core/Core.h"
#include "Input/InputManager.h"
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
#include "Camera/CameraInfo.h"
#include "Component/CameraComponent.h"

IViewportClient::IViewportClient(UCameraComponent* InCameraComponent)
{
	DefaultCamera = InCameraComponent;
}

void IViewportClient::Attach(FCore* Core, FRenderer* Renderer)
{
}

void IViewportClient::Detach(FCore* Core, FRenderer* Renderer)
{
}

void IViewportClient::Tick(FCore* Core, float DeltaTime)
{
	// instead Enhance input system controller
	//if (!Core)
	//{
	//	return;
	//}

	//FInputManager* InputManager = Core->GetInputManager();
	//ULevel* Level = ResolveLevel(Core);
	//if (!InputManager || !Level)
	//{
	//	return;
	//}

	//FCamera* Camera = Level->GetCamera();
	//if (!Camera)
	//{
	//	return;
	//}

	//if (InputManager->IsKeyDown('W')) Camera->MoveForward(DeltaTime);
	//if (InputManager->IsKeyDown('S')) Camera->MoveForward(-DeltaTime);
	//if (InputManager->IsKeyDown('D')) Camera->MoveRight(DeltaTime);
	//if (InputManager->IsKeyDown('A')) Camera->MoveRight(-DeltaTime);
	//if (InputManager->IsKeyDown('E')) Camera->MoveUp(DeltaTime);
	//if (InputManager->IsKeyDown('Q')) Camera->MoveUp(-DeltaTime);

	//if (InputManager->IsMouseButtonDown(FInputManager::MOUSE_RIGHT))
	//{
	//	const float DeltaX = InputManager->GetMouseDeltaX();
	//	const float DeltaY = InputManager->GetMouseDeltaY();
	//	Camera->Rotate(DeltaX * 0.2f, -DeltaY * 0.2f);
	//}
}

void IViewportClient::HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
}

ULevel* IViewportClient::ResolveLevel(FCore* Core) const
{
	return Core ? Core->GetActiveLevel() : nullptr;
}

UWorld* IViewportClient::ResolveWorld(FCore* Core) const
{
	return Core ? Core->GetActiveWorld() : nullptr;
}

void IViewportClient::BuildRenderCommands(FCore* Core, ULevel* Level, const FFrustum& Frustum, FRenderCommandQueue& OutQueue)
{
	UWorld* World = ResolveWorld(Core);
	if (!World) return;

	// Persistent + Streaming 전체 액터를 렌더
	TArray<AActor*> AllActors = World->GetAllActors();
	RenderCollector.CollectRenderCommands(AllActors, Frustum, ShowFlags, OutQueue);
}

void IViewportClient::HandleFileDoubleClick(const FString& FilePath)
{

}

void IViewportClient::HandleFileDropOnViewport(const FString& FilePath)
{

}

void IViewportClient::SetActiveCamera(UCameraComponent* InCamera)
{
	
	ActiveCamera = InCamera;

	if (ActiveCamera == nullptr)
	{
		ActiveCamera = DefaultCamera;
	}
	
}

const FCameraViewInfo& IViewportClient::GetCameraViewInfo() const
{
	return (ActiveCamera == nullptr) ? DefaultCamera->GetViewInfo() : ActiveCamera->GetViewInfo();
}

void IViewportClient::OnViewportResied(uint32 InWidth, uint32 InHeight)
{
	ActiveCamera->SetAspectRatio(static_cast<float>(InWidth) / static_cast<float>(InHeight));
}

void FGameViewportClient::Attach(FCore* Core, FRenderer* Renderer)
{
	if (Renderer)
	{
		Renderer->ClearViewportCallbacks();
	}
}

void FGameViewportClient::Detach(FCore* Core, FRenderer* Renderer)
{
	if (Renderer)
	{
		Renderer->ClearViewportCallbacks();
	}
}
