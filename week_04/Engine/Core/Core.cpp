#include "Core.h"

#include "Object/Scene/Scene.h"
#include "Renderer/Renderer.h"

CCore::~CCore()
{
	Release();
}

bool CCore::Initialize(const HWND Hwnd)
{
	//Renderer = new CRenderer();
	//Renderer->Initialize(Hwnd, WindowWidth, WindowHeight);

	// TODO : Scene 임시 생성
	Scene = new UScene(UScene::StaticClass(), "First Scene");

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

	//	Scene->MarkPendingKill();
	// TODO : Object Manager 가 완성 되면 수정
	delete Scene;
	Scene = nullptr;
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
	if (Scene)
	{
		Scene->Tick(DeltaTime);
	}
}

void CCore::Render()
{
	
}

void CCore::RegisterObjects()
{
}
