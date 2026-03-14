#include "Core.h"

#include "Object/Scene/Scene.h"
#include "Object/Actor/Actor.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderManager.h"
#include "Camera/Camera.h"
#include "Component/PrimitiveComponent.h"
#include "Primitive/PrimitiveBase.h"

CCore::~CCore()
{
	Release();
}

bool CCore::Initialize(HWND Hwnd, int Width, int Height)
{
	WindowWidth = Width;
	WindowHeight = Height;

	// Renderer
	Renderer = new CRenderer();
	if (!Renderer->Initialize(Hwnd, Width, Height))
	{
		return false;
	}

	// ShaderManager
	ShaderManager = new CShaderManager();
	if (!ShaderManager->LoadVertexShader(Renderer->GetDevice(), L"../Engine/Shaders/VertexShader.hlsl"))
	{
		return false;
	}
	if (!ShaderManager->LoadPixelShader(Renderer->GetDevice(), L"../Engine/Shaders/PixelShader.hlsl"))
	{
		return false;
	}

	// Scene
	Scene = new UScene(UScene::StaticClass(), "DefaultScene");
	Scene->InitializeDefaultScene(static_cast<float>(Width) / static_cast<float>(Height));

	return true;
}

void CCore::Release()
{
	delete Scene;
	Scene = nullptr;

	if (ShaderManager)
	{
		ShaderManager->Release();
		delete ShaderManager;
		ShaderManager = nullptr;
	}

	CPrimitiveBase::ClearCache();

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
	if (Scene)
	{
		Scene->Tick(DeltaTime);
	}
}

void CCore::Render()
{
	if (!Renderer || !Scene)
	{
		return;
	}

	Renderer->BeginFrame();

	ShaderManager->Bind(Renderer->GetDeviceContext());

	CCamera* Camera = Scene->GetCamera();
	if (Camera)
	{
		FMatrix VP = Camera->GetViewMatrix() * Camera->GetProjectionMatrix();
		Renderer->ViewProjectionMatrix = VP;
	}

	for (AActor* Actor : Scene->GetActors())
	{
		if (!Actor || Actor->IsPendingDestroy())
		{
			continue;
		}

		for (UActorComponent* Comp : Actor->GetComponents())
		{
			UPrimitiveComponent* PrimComp = dynamic_cast<UPrimitiveComponent*>(Comp);
			if (PrimComp && PrimComp->GetPrimitive())
			{
				Renderer->AddCommand({
					PrimComp->GetPrimitive()->GetMeshData(),
					PrimComp->GetWorldTransform()
				});
			}
		}
	}

	Renderer->ExecuteCommands();
	Renderer->EndFrame();
}
