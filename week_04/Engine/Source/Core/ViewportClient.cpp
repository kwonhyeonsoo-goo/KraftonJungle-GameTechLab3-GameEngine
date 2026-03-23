#include "ViewportClient.h"

#include "Core/Core.h"
#include "Input/InputManager.h"
#include "Camera/Camera.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderCommand.h"
#include "Scene/Scene.h"

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

void IViewportClient::BuildRenderCommands(CCore* Core, UScene* Scene, const FFrustum& Frustum, FRenderCommandQueue& OutQueue) const
{
	if (Scene)
	{
		Scene->CollectRenderCommands(Frustum, OutQueue);
	}
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
