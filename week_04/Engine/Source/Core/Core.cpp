#include "Core.h"
#include "Core/Paths.h"

#include "Object/Scene/Scene.h"
#include "Object/Actor/Actor.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderManager.h"
#include "Input/InputManager.h"
#include "Camera/Camera.h"
#include "Component/PrimitiveComponent.h"
#include "Primitive/PrimitiveBase.h"
#include "Math/Frustum.h"

CCore::~CCore()
{
	Release();
}



bool CCore::Initialize(HWND Hwnd, int32 Width, int32 Height)
{
	FPaths::Initialize();

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

	// InputManager
	InputManager = new CInputManager();

	// Timer
	Timer.Initialize();

	// Scene
	Scene = new UScene(UScene::StaticClass(), "DefaultScene");
	Scene->InitializeDefaultScene(static_cast<float>(Width) / static_cast<float>(Height), Renderer->GetDevice());

	return true;
}

void CCore::ProcessInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (InputManager)
	{
		InputManager->ProcessMessage(Hwnd, Msg, WParam, LParam);
	}
}

void CCore::Release()
{
	delete Scene;
	Scene = nullptr;

	delete InputManager;
	InputManager = nullptr;

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

void CCore::Tick()
{
	Timer.Tick();
	Tick(Timer.GetDeltaTime());
}

void CCore::Tick(const float DeltaTime)
{
	if (InputManager)
	{
		InputManager->Tick();
	}

	ProcessCameraInput(DeltaTime);

	Physics(DeltaTime);
	GameLogic(DeltaTime);
	Render();
}

void CCore::ProcessCameraInput(float DeltaTime)
{
	if (!InputManager || !Scene)
		return;

	CCamera* Camera = Scene->GetCamera();
	if (!Camera)
		return;

	if (InputManager->IsKeyDown('W')) Camera->MoveForward(DeltaTime);
	if (InputManager->IsKeyDown('S')) Camera->MoveForward(-DeltaTime);
	if (InputManager->IsKeyDown('D')) Camera->MoveRight(DeltaTime);
	if (InputManager->IsKeyDown('A')) Camera->MoveRight(-DeltaTime);
	if (InputManager->IsKeyDown('E')) Camera->MoveUp(DeltaTime);
	if (InputManager->IsKeyDown('Q')) Camera->MoveUp(-DeltaTime);

	if (InputManager->IsMouseButtonDown(CInputManager::MOUSE_RIGHT))
	{
		float DeltaX = InputManager->GetMouseDeltaX();
		float DeltaY = InputManager->GetMouseDeltaY();
		Camera->Rotate(DeltaX * 0.2f, -DeltaY * 0.2f);
	}
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
	if (!Renderer || !Scene || Renderer->IsOccluded())
	{
		return;
	}

	Renderer->BeginFrame();

	ShaderManager->Bind(Renderer->GetDeviceContext());

	CCamera* Camera = Scene->GetCamera();
	FFrustum Frustum;
	if (Camera)
	{
		FMatrix V = Camera->GetViewMatrix();
		FMatrix P = Camera->GetProjectionMatrix();
		FMatrix VP = V * P;
		Renderer->SetViewMatrix(V);
		Renderer->SetProjectionMatrix(P);
		Frustum.ExtractFromVP(VP);
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
				if (!Frustum.IsVisible(PrimComp->GetWorldBounds()))
				{
					continue;
				}

				Renderer->AddCommand({
					PrimComp->GetPrimitive()->GetMeshData(),
					PrimComp->GetWorldTransform(),
					PrimComp->GetMaterial()
					});
			}
		}
	}

	Renderer->ExecuteCommands();

	if (PostRenderCallback)
	{
		PostRenderCallback(Renderer);
	}

	Renderer->EndFrame();
}
void CCore::OnResize(int32 Width, int32 Height)
{
	if (Width == 0 || Height == 0) return;

	WindowWidth = Width;
	WindowHeight = Height;


	if (Renderer)
	{
		Renderer->OnResize(Width, Height);
	}


	if (Scene && Scene->GetCamera())
	{
		float NewAspect = static_cast<float>(Width) / static_cast<float>(Height);
		Scene->GetCamera()->SetAspectRatio(NewAspect);
	}
}