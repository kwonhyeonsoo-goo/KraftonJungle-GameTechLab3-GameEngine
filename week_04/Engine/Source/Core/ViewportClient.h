#pragma once

#include "EngineAPI.h"
#include "Windows.h"

class CCore;
class CRenderer;
class UScene;
class FFrustum;
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
};

class ENGINE_API CGameViewportClient : public IViewportClient
{
public:
	void Attach(CCore* Core, CRenderer* Renderer) override;
	void Detach(CCore* Core, CRenderer* Renderer) override;
};
