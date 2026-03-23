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
void IViewportClient::BuildRenderCommands(CCore* Core, UScene* Scene, const FFrustum& Frustum, FRenderCommandQueue& OutQueue) const
{
	if (!Scene)
	{
		UE_LOG("[IViewportClient] Cannot find Scene\n");
		return;
	}

	if (!ShowFlags.HasFlag(EEngineShowFlags::SF_Primitives))
	{
		return;
	}
	TArray<UPrimitiveComponent*> VisiblePrimitives;
	Scene->FrustrumCull(Frustum, VisiblePrimitives);

	for (UPrimitiveComponent* PrimitiveComponent : VisiblePrimitives)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}
		if (PrimitiveComponent->IsA(UUUIDBillboardComponent::StaticClass()))
		{
			UUUIDBillboardComponent* UUIDComponent =
				static_cast<UUUIDBillboardComponent*>(PrimitiveComponent);

			FTextRenderCommand TextCmd;
			TextCmd.Text = UUIDComponent->GetDisplayText();
			TextCmd.WorldPosition = UUIDComponent->GetTextWorldPosition();
			TextCmd.WorldScale = UUIDComponent->GetWorldScale();
			TextCmd.Color = UUIDComponent->GetTextColor();

			OutQueue.AddTextCommand(TextCmd);
			continue;
		}
		if (PrimitiveComponent->IsA(USubUVComponent::StaticClass()))
		{
			USubUVComponent* SubUVComponent = static_cast<USubUVComponent*>(PrimitiveComponent);
			FSubUVRenderCommand SubUVCmd;
			SubUVCmd.WorldMatrix = SubUVComponent->GetWorldTransform();
			SubUVCmd.Size = SubUVComponent->GetSize();
			SubUVCmd.Columns = SubUVComponent->GetColumns();
			SubUVCmd.Rows = SubUVComponent->GetRows();
			SubUVCmd.TotalFrames = SubUVComponent->GetTotalFrames();
			SubUVCmd.FPS = SubUVComponent->GetFPS();
			SubUVCmd.ElapsedTime = static_cast<float>(GEngine->GetCore()->GetTimer().GetTotalTime());
			SubUVCmd.bLoop = SubUVComponent->IsLoop();
			SubUVCmd.bBillboard = SubUVComponent->IsBillboard();
			SubUVCmd.FirstFrame = SubUVComponent->GetFirstFrame();
			SubUVCmd.LastFrame = SubUVComponent->GetLastFrame();

			OutQueue.AddSubUVCommand(SubUVCmd);
			continue;
		}

		if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData())
		{
			continue;
		}

		FRenderCommand Command = BuildRenderCommand(PrimitiveComponent);
		OutQueue.AddCommand(Command);
	}
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
