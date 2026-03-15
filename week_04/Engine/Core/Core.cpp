#include "Core.h"

#include "Object/Scene/Scene.h"
#include "Object/Actor/Actor.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderManager.h"
#include "Input/InputManager.h"
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

	// InputManager
	InputManager = new CInputManager();

	// Timer
	Timer.Initialize();

	// Scene
	Scene = new UScene(UScene::StaticClass(), "DefaultScene");
	Scene->InitializeDefaultScene(static_cast<float>(Width) / static_cast<float>(Height));

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
	if (InputManager && Scene)
	{
		InputManager->Tick(DeltaTime, Scene->GetCamera());
	}

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
	if (!Renderer || !Scene || Renderer->IsOccluded())
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

	// 선택된 Actor 아웃라인 렌더링
	if (SelectedActor && !SelectedActor->IsPendingDestroy())
	{
		for (UActorComponent* Comp : SelectedActor->GetComponents())
		{
			UPrimitiveComponent* PrimComp = dynamic_cast<UPrimitiveComponent*>(Comp);
			if (PrimComp && PrimComp->GetPrimitive())
			{
				Renderer->RenderOutline(
					PrimComp->GetPrimitive()->GetMeshData(),
					PrimComp->GetWorldTransform()
				);
			}
		}
	}

	// 월드 원점 축 렌더링 (X=빨강, Y=초록, Z=파랑)
	float AxisLength = 10000.0f;
	FVector Origin = { 0.0f, 0.0f, 0.0f };
	Renderer->DrawLine(Origin, { AxisLength, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }); // X: Red
	Renderer->DrawLine(Origin, { 0.0f, AxisLength, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }); // Y: Green
	Renderer->DrawLine(Origin, { 0.0f, 0.0f, AxisLength }, { 0.0f, 0.0f, 1.0f, 1.0f }); // Z: Blue
	Renderer->ExecuteLineCommands();

	Renderer->EndFrame();
}
void CCore::OnResize(int Width, int Height)
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