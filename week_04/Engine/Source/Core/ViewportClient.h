#pragma once

#include "EngineAPI.h"
#include "Windows.h"
#include "Types/String.h"
#include "ShowFlags.h"
#include "Renderer/RenderCommand.h"
#include "World/RenderCollector.h"

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
	virtual ~IViewportClient() = default;

	virtual void Attach(FCore* Core, FRenderer* Renderer);
	virtual void Detach(FCore* Core, FRenderer* Renderer);
	virtual void Tick(FCore* Core, float DeltaTime);
	virtual void HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);
	virtual ULevel* ResolveLevel(FCore* Core) const;
	virtual UWorld* ResolveWorld(FCore* Core) const;
	FShowFlags& GetShowFlags() { return ShowFlags; }
	const FShowFlags& GetShowFlags() const { return ShowFlags; }
	virtual void BuildRenderCommands(FCore* Core, ULevel* Level,
		const FFrustum& Frustum, FRenderCommandQueue& OutQueue);
	/** 입력 처리는 원래 Viewport 에서 처리하는게 맞는데 구조상 여기다 넣음 */
	virtual void HandleFileDoubleClick(const FString& FilePath);
	virtual void HandleFileDropOnViewport(const FString& FilePath);
protected:
	FShowFlags ShowFlags;
	FLevelRenderCollector RenderCollector;
};

class ENGINE_API FGameViewportClient : public IViewportClient
{
public:
	void Attach(FCore* Core, FRenderer* Renderer) override;
	void Detach(FCore* Core, FRenderer* Renderer) override;
};
