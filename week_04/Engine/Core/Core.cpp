#include "Core.h"

#include "Renderer/Renderer.h"

CCore::~CCore()
{
	Release();
}

bool CCore::Initialize(const HWND Hwnd)
{
	Renderer = new CRenderer();
	Renderer->Initialize(Hwnd, WindowWidth, WindowHeight);

	return true;
}

void CCore::Release()
{
	if (Renderer)
	{
		Renderer->Release();
		delete Renderer;
		Renderer = nullptr;
	}
}

void CCore::Tick(const float DeltaTime)
{
	Physics(DeltaTime);
	GameLogic(DeltaTime);
	Render();
}

void CCore::Physics(float DeltaTime)
{

}

void CCore::GameLogic(float DeltaTime)
{

}

void CCore::Render()
{

}

void CCore::RegisterObjects()
{
}
