#pragma once

#include "EngineAPI.h"
#include "Windows.h"
#include "Types/String.h"
#include "ShowFlags.h"
#include "Renderer/RenderCommand.h"
#include "World/RenderCollector.h"
#include "Camera/CameraInfo.h"
#include "Camera/ViewportInfo.h"
#include "Component/CameraComponent.h"

class FCore;
class FRenderer;
class ULevel;
class FFrustum;
class UPrimitiveComponent;
struct FRenderCommandQueue;
class UWorld;

class ENGINE_API IViewportClient
{
public:
	IViewportClient();
	virtual ~IViewportClient() = default;

	virtual void Attach(FCore* Core, FRenderer* Renderer);
	virtual void Detach(FCore* Core, FRenderer* Renderer);
	virtual void Tick(FCore* Core, float DeltaTime);
	virtual void HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);

	// ===== [world] =====

	virtual ULevel* ResolveLevel(FCore* Core) const;
	virtual UWorld* ResolveWorld(FCore* Core) const;

	void SetLinkedWorld(UWorld InWorld);
	void GetLinkedWorld();


	FShowFlags& GetShowFlags() { return ShowFlags; }
	const FShowFlags& GetShowFlags() const { return ShowFlags; }
	virtual void BuildRenderCommands(FCore* Core, ULevel* Level,
		const FFrustum& Frustum, FRenderCommandQueue& OutQueue);

	virtual void HandleFileDoubleClick(const FString& FilePath);
	virtual void HandleFileDropOnViewport(const FString& FilePath);

	// ── 카메라 ────────────────────────────────────────────────────────
	// nullptr 전달 시 DefaultCamera로 자동 복귀
	void SetActiveCamera(UCameraComponent* InCamera);
	UCameraComponent* GetActiveCamera() const;
	FCameraViewInfo GetCameraViewInfo() const;
	void OnViewportResized(uint32 InWidth, uint32 InHeight);

	// ── 뷰포트 정보 ───────────────────────────────────────────────────
	void SetViewportInfo(const FViewportInfo& InViewportInfo);
	const FViewportInfo& GetViewportInfo() const { return ViewportInfo; }

protected:
	FShowFlags ShowFlags;
	FLevelRenderCollector RenderCollector;

private:
	// 항상 유효, 절대 nullptr 없음
	UCameraComponent  DefaultCamera;
	// 외부 CameraActor 연결용, nullptr이면 DefaultCamera 사용
	UCameraComponent* ActiveCamera = nullptr;

	FViewportInfo ViewportInfo;

	UWorld* LInkedWorld; 
};

class ENGINE_API FGameViewportClient : public IViewportClient
{
public:
	void Attach(FCore* Core, FRenderer* Renderer) override;
	void Detach(FCore* Core, FRenderer* Renderer) override;
};