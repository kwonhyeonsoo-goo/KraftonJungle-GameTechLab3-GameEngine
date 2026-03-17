#include "Core.h"

#include "Scene/Scene.h"
#include "Actor/Actor.h"
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

bool CCore::CreateSceneContext(FSceneContext& Context, const FString& ContextName, ESceneType SceneType, float AspectRatio, bool bInitializeDefaultScene)
{
	Context.ContextName = ContextName;
	Context.SceneType = SceneType;
	Context.Scene = new UScene(UScene::StaticClass(), ContextName);
	if (!Context.Scene)
	{
		return false;
	}

	Context.Scene->SetSceneType(SceneType);
	if (bInitializeDefaultScene)
	{
		Context.Scene->InitializeDefaultScene(AspectRatio);
	}
	else
	{
		Context.Scene->InitializeEmptyScene(AspectRatio);
	}
	return true;
}

void CCore::DestroySceneContext(FSceneContext& Context)
{
	delete Context.Scene;
	Context.Reset();
}

void CCore::DestroySceneContext(FEditorSceneContext& Context)
{
	delete Context.Scene;
	Context.Reset();
}

FEditorSceneContext* CCore::GetActiveEditorSceneContext()
{
	if (ActiveSceneContext == &EditorSceneContext)
	{
		return &EditorSceneContext;
	}

	for (const std::unique_ptr<FEditorSceneContext>& Context : PreviewSceneContexts)
	{
		if (Context && Context.get() == ActiveSceneContext)
		{
			return Context.get();
		}
	}

	return nullptr;
}

const FEditorSceneContext* CCore::GetActiveEditorSceneContext() const
{
	if (ActiveSceneContext == &EditorSceneContext)
	{
		return &EditorSceneContext;
	}

	for (const std::unique_ptr<FEditorSceneContext>& Context : PreviewSceneContexts)
	{
		if (Context && Context.get() == ActiveSceneContext)
		{
			return Context.get();
		}
	}

	return nullptr;
}

FEditorSceneContext* CCore::FindPreviewSceneContext(const FString& ContextName)
{
	for (const std::unique_ptr<FEditorSceneContext>& Context : PreviewSceneContexts)
	{
		if (Context && Context->ContextName == ContextName)
		{
			return Context.get();
		}
	}

	return nullptr;
}

const FEditorSceneContext* CCore::FindPreviewSceneContext(const FString& ContextName) const
{
	for (const std::unique_ptr<FEditorSceneContext>& Context : PreviewSceneContexts)
	{
		if (Context && Context->ContextName == ContextName)
		{
			return Context.get();
		}
	}

	return nullptr;
}

bool CCore::Initialize(HWND Hwnd, int32 Width, int32 Height, ESceneType StartupSceneType)
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

	const float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
	FSceneContext* StartupContext = &GameSceneContext;
	FString ContextName = "GameScene";

	if (StartupSceneType == ESceneType::Editor)
	{
		StartupContext = &EditorSceneContext;
		ContextName = "EditorScene";
	}

	if (!CreateSceneContext(*StartupContext, ContextName, StartupSceneType, AspectRatio))
	{
		return false;
	}

	ActiveSceneContext = StartupContext;

	return true;
}

UScene* CCore::GetPreviewScene(const FString& ContextName) const
{
	const FEditorSceneContext* Context = FindPreviewSceneContext(ContextName);
	return Context ? Context->Scene : nullptr;
}

void CCore::SetSelectedActor(AActor* InActor)
{
	FEditorSceneContext* ActiveEditorContext = GetActiveEditorSceneContext();
	if (ActiveEditorContext)
	{
		ActiveEditorContext->SelectedActor = InActor;
		return;
	}

	EditorSceneContext.SelectedActor = InActor;
}

AActor* CCore::GetSelectedActor() const
{
	const FEditorSceneContext* ActiveEditorContext = GetActiveEditorSceneContext();
	if (ActiveEditorContext)
	{
		return ActiveEditorContext->SelectedActor;
	}

	return EditorSceneContext.SelectedActor;
}

FEditorSceneContext* CCore::CreatePreviewSceneContext(const FString& ContextName)
{
	if (ContextName.empty())
	{
		return nullptr;
	}

	if (FEditorSceneContext* ExistingContext = FindPreviewSceneContext(ContextName))
	{
		return ExistingContext;
	}

	std::unique_ptr<FEditorSceneContext> PreviewContext = std::make_unique<FEditorSceneContext>();
	const float AspectRatio = (WindowHeight > 0) ? (static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight)) : 1.0f;
	if (!CreateSceneContext(*PreviewContext, ContextName, ESceneType::Preview, AspectRatio, false))
	{
		return nullptr;
	}

	FEditorSceneContext* CreatedContext = PreviewContext.get();
	PreviewSceneContexts.push_back(std::move(PreviewContext));
	return CreatedContext;
}

bool CCore::DestroyPreviewSceneContext(const FString& ContextName)
{
	for (auto It = PreviewSceneContexts.begin(); It != PreviewSceneContexts.end(); ++It)
	{
		if (*It && (*It)->ContextName == ContextName)
		{
			if (ActiveSceneContext == It->get())
			{
				ActivateEditorScene();
				if (ActiveSceneContext == nullptr)
				{
					ActivateGameScene();
				}
			}

			DestroySceneContext(*(*It));
			PreviewSceneContexts.erase(It);
			return true;
		}
	}

	return false;
}

bool CCore::ActivatePreviewScene(const FString& ContextName)
{
	FEditorSceneContext* PreviewContext = FindPreviewSceneContext(ContextName);
	if (PreviewContext == nullptr)
	{
		return false;
	}

	ActiveSceneContext = PreviewContext;
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
	ActiveSceneContext = nullptr;
	for (std::unique_ptr<FEditorSceneContext>& PreviewContext : PreviewSceneContexts)
	{
		if (PreviewContext)
		{
			DestroySceneContext(*PreviewContext);
		}
	}
	PreviewSceneContexts.clear();
	DestroySceneContext(EditorSceneContext);
	DestroySceneContext(GameSceneContext);

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
	UScene* Scene = GetActiveScene();
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
	UScene* Scene = GetActiveScene();
	if (Scene)
	{
		Scene->Tick(DeltaTime);
	}
}

void CCore::Render()
{
	UScene* Scene = GetActiveScene();
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
		FMatrix VP = Camera->GetViewMatrix() * Camera->GetProjectionMatrix();
		Renderer->ViewProjectionMatrix = VP;
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
					PrimComp->GetWorldTransform()
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

	UScene* Scene = GetActiveScene();
	if (Scene && Scene->GetCamera())
	{
		float NewAspect = static_cast<float>(Width) / static_cast<float>(Height);
		Scene->GetCamera()->SetAspectRatio(NewAspect);
	}
}
