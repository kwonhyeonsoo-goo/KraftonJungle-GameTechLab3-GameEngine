#pragma once

#include "EngineAPI.h"
#include "Windows.h"
#include "Types/String.h"
#include "ShowFlags.h"
#include "Renderer/RenderCommand.h"


class CCore;
class CRenderer;
class UScene;
class FFrustum;
class UPrimitiveComponent;
struct FRenderCommandQueue;

class ENGINE_API IViewportClient
{
public:
	virtual ~IViewportClient() = default;

	virtual void Attach(CCore* Core, CRenderer* Renderer);
	virtual void Detach(CCore* Core, CRenderer* Renderer);
	virtual void Tick(CCore* Core, float DeltaTime);
	virtual void HandleMessage(CCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);
	virtual UScene* ResolveScene(CCore* Core) const;
	virtual void BuildRenderCommands(CCore* Core, UScene* Scene, const FFrustum& Frustum, FRenderCommandQueue& OutQueue) const;
	FShowFlags& GetShowFlags() { return ShowFlags; }
	const FShowFlags& GetShowFlags() const { return ShowFlags; }
	
	/** 입력 처리는 원래 Viewport 에서 처리하는게 맞는데 구조상 여기다 넣음 */
	virtual void HandleFileDoubleClick(const FString& FilePath);
protected:
	FShowFlags ShowFlags;

	virtual FRenderCommand BuildRenderCommand(UPrimitiveComponent* PrimitiveComponent) const;
};

class ENGINE_API CGameViewportClient : public IViewportClient
{
public:
	void Attach(CCore* Core, CRenderer* Renderer) override;
	void Detach(CCore* Core, CRenderer* Renderer) override;
};
