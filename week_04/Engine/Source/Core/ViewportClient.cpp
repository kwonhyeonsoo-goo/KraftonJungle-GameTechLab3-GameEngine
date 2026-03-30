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

