#include "ViewportClient.h"
#include "World/World.h"
#include "Core/Core.h"
#include "Input/InputManager.h"
#include "Camera/Camera.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/Material.h"
#include "Scene/Scene.h"
#include "Debug/EngineLog.h"
#include "Component/UUIDBillboardComponent.h"
#include "Component/SubUVComponent.h"
#include "Core/FEngine.h"


void IViewportClient::Attach(CCore* Core, CRenderer* Renderer)
{
}

void IViewportClient::Detach(CCore* Core, CRenderer* Renderer)
{
}

void IViewportClient::Tick(CCore* Core, float DeltaTime)
{
	// instead Enhance input system controller
	//if (!Core)
	//{
	//	return;
	//}

	//CInputManager* InputManager = Core->GetInputManager();
	//UScene* Scene = ResolveScene(Core);
	//if (!InputManager || !Scene)
	//{
	//	return;
	//}

	//CCamera* Camera = Scene->GetCamera();
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

	//if (InputManager->IsMouseButtonDown(CInputManager::MOUSE_RIGHT))
	//{
	//	const float DeltaX = InputManager->GetMouseDeltaX();
	//	const float DeltaY = InputManager->GetMouseDeltaY();
	//	Camera->Rotate(DeltaX * 0.2f, -DeltaY * 0.2f);
	//}
}

void IViewportClient::HandleMessage(CCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
}

UScene* IViewportClient::ResolveScene(CCore* Core) const
{
	return Core ? Core->GetActiveScene() : nullptr;
}

UWorld* IViewportClient::ResolveWorld(CCore* Core) const
{
	return Core ? Core->GetActiveWorld() : nullptr;
}


FRenderCommand IViewportClient::BuildRenderCommand(UPrimitiveComponent* PrimitiveComponent) const
{
	FRenderCommand Command;
	Command.RenderLayer = ERenderLayer::Default;
	Command.MeshData = PrimitiveComponent->GetPrimitive()->GetMeshData();
	Command.WorldMatrix = PrimitiveComponent->GetWorldTransform();
	Command.Material = PrimitiveComponent->GetMaterial();
	return Command;
}

void IViewportClient::BuildRenderCommands(CCore* Core, UScene* Scene, const FFrustum& Frustum, FRenderCommandQueue& OutQueue)
{
	if (!Scene)
	{
		UE_LOG("[IViewportClient] Cannot find Scene\n");
		return;
	}
	RenderCollector.CollectRenderCommands(Scene->GetActors(), Frustum, ShowFlags, OutQueue);
}

void IViewportClient::HandleFileDoubleClick(const FString& FilePath)
{

}

void CGameViewportClient::Attach(CCore* Core, CRenderer* Renderer)
{
	if (Renderer)
	{
		Renderer->ClearViewportCallbacks();
	}
}

void CGameViewportClient::Detach(CCore* Core, CRenderer* Renderer)
{
	if (Renderer)
	{
		Renderer->ClearViewportCallbacks();
	}
}
