#include "ViewportClient.h"
#include "World/World.h"
#include "Core/Core.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/Material.h"
#include "World/Level.h"
#include "Debug/EngineLog.h"
#include "Component/UUIDBillboardComponent.h"
#include "Component/SubUVComponent.h"
#include "Core/FEngine.h"
#include "Component/TextComponent.h"
#include "Actor/CameraPawn.h"
#include "Core/ShowFlags.h"

// ── 생성자 ─────────────────────────────────────────────────────────────────

IViewportClient::IViewportClient()
{
	// UX 고려 전 임시 초기값 — 이후 별도 설정
	DefaultCamera.SetPosition({ -5.0f, 0.0f, 2.0f });
	DefaultCamera.SetRotation(0.0f, 0.0f);
}

// ── 카메라 ─────────────────────────────────────────────────────────────────

void IViewportClient::SetActiveCamera(UCameraComponent* InCamera)
{
	// 최초 등록에 대한 logic이 부족함
	ActiveCamera = InCamera; // nullptr이면 GetActiveCamera()에서 DefaultCamera 반환
}

UCameraComponent* IViewportClient::GetActiveCamera() const
{
	return ActiveCamera ? ActiveCamera : const_cast<UCameraComponent*>(&DefaultCamera);
}

FCameraViewInfo IViewportClient::GetCameraViewInfo() const
{
	// GetActiveCamera()는 항상 유효한 포인터 보장
	return GetActiveCamera()->GetViewInfo();
}

void IViewportClient::OnViewportResized(uint32 InWidth, uint32 InHeight)
{
	if (InWidth == 0 || InHeight == 0)
	{
		return;
	}
	// GetActiveCamera()를 통해 DefaultCamera/ActiveCamera 구분 없이 동일하게 반영
	GetActiveCamera()->SetAspectRatio(static_cast<float>(InWidth) / static_cast<float>(InHeight));
}

void IViewportClient::InitializeCameraFromWorld()
{

	UCameraPawn* CameraPawn = LinkedWorld->SpawnActor<UCameraPawn>("CameraPawn");
	SetActiveCamera(CameraPawn->GetCameraComponent());

}
void IViewportClient::SetCameraMode(CameraViewMode mode)
{
	UCameraComponent* Cam = ActiveCamera;
	if (!Cam) return;

	switch (mode)
	{

	case CameraViewMode::Perspective:
		Cam->SetPosition({ -10.f, 0.f, 5.f });
		Cam->SetRotation(0.f, -15.f);
		Cam->SetFOV(60.f);
		Cam->SetOrthographic(false);
		break;
	case CameraViewMode::Top:
		Cam->SetPosition({ 0.f, 0.f, 100.f });
		Cam->SetRotation(0.f, -90.f);   // 거의 수직 아래
		Cam->SetOrthographic(true);
		Cam->SetOrthoWidth(15.f);
		break;
	case CameraViewMode::Bottom:
		Cam->SetPosition({ 0.f, 0.f,  -100.f });
		Cam->SetRotation(0.f, 90.f);    
		Cam->SetOrthographic(true);
		Cam->SetOrthoWidth(15.f);
		break;
	case CameraViewMode::Left:
		Cam->SetPosition({ 0.f, -100.f, 2.f });
		Cam->SetRotation(90.f, 0.f);     // +Y 방향 바라봄
		Cam->SetOrthographic(true);
		Cam->SetOrthoWidth(15.f);
		break;
	case CameraViewMode::Right:
		Cam->SetPosition({ 0.f, 100.f, 2.f });
		Cam->SetRotation(-90.f, 0.f);    // -Y 방향(앞쪽 기준) 바라봄
		Cam->SetOrthographic(true);
		Cam->SetOrthoWidth(15.f);
		break;
	case CameraViewMode::Front:
		Cam->SetPosition({ 100.f, 0.f, 2.f });
		Cam->SetRotation(180.f, 0.f);    // 왼쪽 방향
		Cam->SetOrthographic(true);
		Cam->SetOrthoWidth(15.f);
		
		break;
	case CameraViewMode::Back:

		Cam->SetPosition({ -100.f, 0.f, 2.f });
		Cam->SetRotation(0.f, 0.f);      // 오른쪽(+X) 방향을 바라봄
		Cam->SetOrthographic(true);
		Cam->SetOrthoWidth(15.f);
		break;
	default:
		break;
	}
}
// ── 뷰포트 정보 ────────────────────────────────────────────────────────────

void IViewportClient::SetViewportInfo(const FViewportInfo& InViewportInfo)
{
	ViewportInfo = InViewportInfo;

	// 크기 변경 시 AspectRatio 자동 동기화
	if (InViewportInfo.Width > 0.f && InViewportInfo.Height > 0.f)
	{
		OnViewportResized(
			static_cast<uint32>(InViewportInfo.Width),
			static_cast<uint32>(InViewportInfo.Height));
	}
}

// ── 라이프사이클 ───────────────────────────────────────────────────────────

void IViewportClient::Attach(FCore* Core, FRenderer* Renderer)
{
}

void IViewportClient::Detach(FCore* Core, FRenderer* Renderer)
{
}

void IViewportClient::Tick(FCore* Core, float DeltaTime)
{
}

void IViewportClient::HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
}

ULevel* IViewportClient::ResolveLevel(FCore* Core) const
{
	return LinkedWorld ? LinkedWorld->GetPersistentLevel() : Core->GetActiveLevel();
}

UWorld* IViewportClient::ResolveWorld(FCore* Core) const
{	
	return LinkedWorld ? LinkedWorld : Core->GetActiveWorld();
}

void IViewportClient::BuildRenderCommands(FCore* Core, ULevel* Level, const FFrustum& Frustum, FRenderCommandQueue& OutQueue)
{
	UWorld* World = ResolveWorld(Core);
	if (!World) return;

	TArray<AActor*> AllActors = World->GetAllActors();
	RenderCollector.CollectRenderCommands(AllActors, Frustum, ShowFlags, OutQueue);
}

void IViewportClient::HandleFileDoubleClick(const FString& FilePath)
{
}

void IViewportClient::HandleFileDropOnViewport(const FString& FilePath)
{
}
// ======== World ==============

void IViewportClient::SetLinkedWorld(UWorld* InWorld , FCore* InCore)
{
	if (InWorld == nullptr)
	{
		LinkedWorld = ResolveWorld(InCore);
		return;
	}
	LinkedWorld = InWorld;

}
UWorld* IViewportClient::GetLinkedWorld()
{
	return LinkedWorld;
}
// ── FGameViewportClient ────────────────────────────────────────────────────

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

