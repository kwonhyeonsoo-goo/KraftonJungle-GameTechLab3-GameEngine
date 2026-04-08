#include "GameViewportClient.h"

#include "Actor/Controller.h"
#include "Component/CameraComponent.h"
#include "Math/Frustum.h"
#include "World/World.h"

void FGameViewportClient::Attach()
{
	ShowFlags.SetFlag(EEngineShowFlags::SF_UUID, false);
	ShowFlags.SetFlag(EEngineShowFlags::SF_Collision, false);
}

void FGameViewportClient::Detach()
{

}

void FGameViewportClient::ProcessCameraInput(float DeltaTime)
{
	AController* Controller = GWorld->GetDefaultController();
	if (Controller)
	{
		for (int32 key = 0; key < 256; ++key)
		{
			if (InputManager->IsKeyDown(key))
			{
				Controller->ProcessInput(key, EInputEventType::KeyDown);
				continue;
			}
			else if (InputManager->IsKeyReleased(key))
			{
				Controller->ProcessInput(key, EInputEventType::KeyUp);
				continue;
			}
		}

		if (InputManager->IsMouseButtonPressed(FInputManager::MOUSE_LEFT))
		{
			Controller->ProcessInput(FInputManager::MOUSE_LEFT, EInputEventType::MouseButtonDown);
		}
		else if (InputManager->IsMouseButtonReleased(FInputManager::MOUSE_LEFT))
		{
			Controller->ProcessInput(FInputManager::MOUSE_LEFT, EInputEventType::MouseButtonUp);
		}

		if (InputManager->IsMouseButtonPressed(FInputManager::MOUSE_RIGHT))
		{
			Controller->ProcessInput(FInputManager::MOUSE_RIGHT, EInputEventType::MouseButtonDown);
		}
		else if (InputManager->IsMouseButtonReleased(FInputManager::MOUSE_RIGHT))
		{
			Controller->ProcessInput(FInputManager::MOUSE_RIGHT, EInputEventType::MouseButtonUp);
		}
	}
}

void FGameViewportClient::BuildRenderCommands(TArray<AActor*>& InActors, FRenderCommandQueue& OutQueue)
{
	FFrustum Frustum;
	const float AspectRatio = ViewportHeight > 0 ? static_cast<float>(ViewportWidth) / static_cast<float>(ViewportHeight) : 1.0f;

	AController* Controller = GWorld->GetDefaultController();
	FCamera* Camera = &CameraTransform;

	if (Controller && Controller->GetCamera())
	{
		Camera = Controller->GetCamera()->GetCamera(); 

		Camera->SetPosition(Controller->GetCamera()->GetWorldLocation());
	}
	Camera->SetAspectRatio(AspectRatio);

	const FMatrix ViewMatrix = Camera->GetViewMatrix();
	const FMatrix ProjectionMatrix = Camera->GetProjectionMatrix();
	const FMatrix ViewProjection = ViewMatrix * ProjectionMatrix;
	Frustum.ExtractFromVP(ViewProjection);

	OutQueue.ViewMatrix = ViewMatrix;
	OutQueue.ProjectionMatrix = ProjectionMatrix;
	RenderCollector.CollectRenderCommands(InActors, Frustum, ShowFlags, Camera, OutQueue);
}