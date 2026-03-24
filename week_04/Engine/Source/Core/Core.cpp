#include "Core.h"

#include "Core/Paths.h"
#include "Core/ConsoleVariableManager.h"
#include "Scene/Scene.h"
#include "Actor/Actor.h"
#include "Input/EnhancedInputManager.h"
#include "Component/CameraComponent.h"
#include "Object/ObjectFactory.h"
#include "Object/ObjectManager.h"
#include "Component/PrimitiveComponent.h"
#include "Primitive/PrimitiveBase.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/MaterialManager.h"
#include "Math/Frustum.h"
#include "Input/InputManager.h"
#include "ViewportClient.h"
#include "Object/ObjectGlobals.h"

CCore::~CCore()
{
	Release();
}

bool CCore::Initialize(HWND Hwnd, int32 Width, int32 Height, ESceneType StartupSceneType)
{
	FPaths::Initialize();
	WindowWidth = Width;
	WindowHeight = Height;

	Renderer = std::make_unique<CRenderer>();
	if (!Renderer->Initialize(Hwnd, Width, Height))
	{
		return false;
	}

	ObjManager = new ObjectManager();

	// Material
	FMaterialManager::Get().LoadAllMaterials(Renderer->GetDevice(), Renderer->GetRenderStateManager().get());

	// InputManager
	InputManager = new CInputManager();
	EnhancedInput = new CEnhancedInputManager();

	PhysicsManager = std::make_unique<CPhysicsManager>();

	// Timer
	Timer.Initialize();
	RegisterConsoleVariables();
	SceneManager = std::make_unique<FSceneManager>();
	const float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
	if (!SceneManager->Initialize(AspectRatio, StartupSceneType, Renderer.get()))
	{
		return false;
	}

	return true;
}



void CCore::SetViewportClient(IViewportClient* InViewportClient)
{
	if (ViewportClient == InViewportClient)
	{
		return;
	}

	if (ViewportClient && Renderer)
	{
		ViewportClient->Detach(this, Renderer.get());
	}

	ViewportClient = InViewportClient;

	if (ViewportClient && Renderer)
	{
		ViewportClient->Attach(this, Renderer.get());
	}
}

void CCore::ProcessInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (InputManager)
	{
		InputManager->ProcessMessage(Hwnd, Msg, WParam, LParam);
	}

	if (ViewportClient)
	{
		ViewportClient->HandleMessage(this, Hwnd, Msg, WParam, LParam);
	}
}

void CCore::Release()
{
	if (ViewportClient && Renderer)
	{
		ViewportClient->Detach(this, Renderer.get());
	}
	ViewportClient = nullptr;
	if (SceneManager)
	{
		SceneManager->Release();
		SceneManager.reset();
	}

	// Scene 해제 후 PendingKill 오브젝트를 GC로 정리
	if (ObjManager)
	{
		ObjManager->FlushKilledObjects();
		delete ObjManager;
		ObjManager = nullptr;
	}

	delete EnhancedInput;
	EnhancedInput = nullptr;
	delete InputManager;
	InputManager = nullptr;
	CPrimitiveBase::ClearCache();

	if (Renderer)
	{
		Renderer->Release();
		Renderer.reset();
	}
}

void CCore::Tick()
{
	Timer.Tick();
	Tick(Timer.GetDeltaTime());
}

void CCore::Tick(const float DeltaTime)
{
	Input(DeltaTime);
	Physics(DeltaTime);
	GameLogic(DeltaTime);
	Render();
	LateUpdate(DeltaTime);
}

void CCore::Input(float DeltaTime)
{
	if (InputManager)
	{
		InputManager->Tick();
	}

	if (EnhancedInput && InputManager)
	{
		EnhancedInput->ProcessInput(InputManager, DeltaTime);
	}

	if (ViewportClient)
	{
		ViewportClient->Tick(this, DeltaTime);
	}
}

void CCore::Physics(float DeltaTime)
{
	UScene* Scene = ViewportClient ? ViewportClient->ResolveScene(this) : GetActiveScene();
	
	if (Scene)
	{
		FVector LineStart(2, 2, 0), LineEnd(5, 5, 0);
		FHitResult HitResult;

		bool bHit = PhysicsManager->Linetrace(Scene, LineStart, LineEnd, HitResult);

		if (bHit)
		{
			for (UActorComponent* ActorComp : HitResult.HitActor->GetComponents())
			{
				if (!ActorComp->IsA(UPrimitiveComponent::StaticClass()))
				{
					continue;
				}

				UPrimitiveComponent* PrimComp = static_cast<UPrimitiveComponent*>(ActorComp);

				if (PrimComp)
				{
					FBoxSphereBounds Bound;
					Bound = PrimComp->GetWorldBounds();
					Renderer->DrawCube(Bound.Center, Bound.BoxExtent, FVector4(1, 0, 0, 1));
				}
			}

			Renderer->DrawCube(HitResult.HitLocation, FVector(0.1, 0.1, 0.1), FVector4(0, 1, 0, 1));
		}

		if (Renderer)
		{
			Renderer->DrawLine(LineStart, LineEnd, FVector4(0, 1, 1, 1));
		}
	}
}

void CCore::GameLogic(float DeltaTime)
{
	UWorld* World = GetActiveWorld();
	if (World)
	{
		World->Tick(DeltaTime);
	}
}

void CCore::LateUpdate(float DeltaTime)
{
	if (GCInterval <= 0.0)
	{
		return;
	}

	double CurrentTime = Timer.GetTotalTime();
	if (ObjManager && (CurrentTime - LastGCTime) >= GCInterval)
	{
		ObjManager->FlushKilledObjects();
		LastGCTime = CurrentTime;
	}
}

void CCore::Render()
{
	UScene* Scene = ViewportClient ? ViewportClient->ResolveScene(this) : GetActiveScene();
	if (!Renderer || !Scene || Renderer->IsOccluded())
	{
		return;
	}

	Renderer->BeginFrame();

	UWorld* ActiveWorld = GetActiveWorld();
	if (!ActiveWorld)
	{
		Renderer->EndFrame();
		return;
	}
	UCameraComponent* ActiveCamera = ActiveWorld->GetActiveCameraComponent();
	if (!ActiveCamera)
	{
		Renderer->EndFrame();
		return;
	}

	CommandQueue.Clear();
	CommandQueue.Reserve(Renderer->GetPrevCommandCount());
	CommandQueue.ViewMatrix = ActiveCamera->GetViewMatrix();
	CommandQueue.ProjectionMatrix = ActiveCamera->GetProjectionMatrix();

	FFrustum Frustum;
	const FMatrix ViewProjection = CommandQueue.ViewMatrix * CommandQueue.ProjectionMatrix;
	Frustum.ExtractFromVP(ViewProjection);

	if (ViewportClient)
	{
		ViewportClient->BuildRenderCommands(this, Scene, Frustum, CommandQueue);
	}
	else
	{
		// Scene->CollectRenderCommands(Frustum, CommandQueue);
	}

	Renderer->SubmitCommands(CommandQueue);
	Renderer->ExecuteCommands();
	Renderer->EndFrame();
}

void CCore::OnResize(int32 Width, int32 Height)
{
	if (Width == 0 || Height == 0) return;
	WindowWidth = Width;
	WindowHeight = Height;
	if (Renderer) Renderer->OnResize(Width, Height);
	if (SceneManager) SceneManager->OnResize(Width, Height);
}

void CCore::RegisterConsoleVariables()
{
	FConsoleVariableManager& CVM = FConsoleVariableManager::Get();

	FConsoleVariable* MaxFPSVar = CVM.Find("t.MaxFPS");
	if (!MaxFPSVar)
	{
		MaxFPSVar = CVM.Register("t.MaxFPS", 0.0f, "Maximum FPS limit (0 = unlimited)");
	}
	MaxFPSVar->SetOnChanged([this](FConsoleVariable* Var)
		{
			Timer.SetMaxFPS(Var->GetFloat());
		});
	Timer.SetMaxFPS(MaxFPSVar->GetFloat());

	FConsoleVariable* VSyncVar = CVM.Find("r.VSync");
	if (!VSyncVar)
	{
		VSyncVar = CVM.Register("r.VSync", 0, "Enable VSync (0 = off, 1 = on)");
	}
	VSyncVar->SetOnChanged([this](FConsoleVariable* Var)
		{
			if (Renderer)
			{
				Renderer->SetVSync(Var->GetInt() != 0);
			}
		});
	if (Renderer)
	{
		Renderer->SetVSync(VSyncVar->GetInt() != 0);
	}

	FConsoleVariable* GCIntervalVar = CVM.Find("gc.Interval");
	if (!GCIntervalVar)
	{
		GCIntervalVar = CVM.Register("gc.Interval", 30.0f, "GC interval in seconds (0 = disabled)");
	}
	GCIntervalVar->SetOnChanged([this](FConsoleVariable* Var)
		{
			GCInterval = static_cast<double>(Var->GetFloat());
		});
	GCInterval = static_cast<double>(GCIntervalVar->GetFloat());

	CVM.RegisterCommand("ForceGC", [this](FString& OutResult)
		{
			if (ObjManager)
			{
				ObjManager->FlushKilledObjects();
				LastGCTime = Timer.GetTotalTime();
				OutResult = "ForceGC: Garbage collection completed.";
			}
			else
			{
				OutResult = "ForceGC: ObjectManager is not available.";
			}
		}, "Force immediate garbage collection");
}

