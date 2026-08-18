#include "GameViewportClient.h"

#include "Actor/Controller.h"
#include "Component/CameraComponent.h"
#include "Input/EnhancedInputManager.h"
#include "Math/Frustum.h"
#include "World/World.h"

void FGameViewportClient::Attach()
{
	ShowFlags.SetFlag(EEngineShowFlags::SF_UUID, false);
	ShowFlags.SetFlag(EEngineShowFlags::SF_Collision, false);

	if (InputManager)
		InputManager->SetGameMode(true);
	//뷰포트가 교체될 때 렌더 캐시를 강제로 비워 첫 프레임 누락을 방지
	RenderCollector.MarkDirty();
}

void FGameViewportClient::Detach()
{
	if (InputManager)
		InputManager->SetGameMode(false);

	// Controller의 입력 바인딩 해제
	AController* Controller = GWorld ? GWorld->GetDefaultController() : nullptr;
	if (Controller)
	{
		Controller->SetupInput(nullptr, nullptr);
	}
}

void FGameViewportClient::ProcessCameraInput(float DeltaTime)
{
	AController* Controller = GWorld->GetDefaultController();
	if (!Controller) return;

	// Controller에 입력 시스템 연결 (최초 1회 또는 재연결 시)
	if (EnhancedInput)
	{
		Controller->SetupInput(InputManager, EnhancedInput);
		EnhancedInput->ProcessInput(InputManager, DeltaTime);
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
