#include "Core.h"

#include "Core/Paths.h"
#include "Core/ConsoleVariableManager.h"
#include "World/Level.h"
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
#include "Component/UUIDBillboardComponent.h"
#include "Component/SubUVComponent.h"
#include "Actor/SkySphereActor.h"
FCore::~FCore()
{
	Release();
}

bool FCore::Initialize(HWND Hwnd, int32 Width, int32 Height, ELevelType StartupLevelType)
{
	FPaths::Initialize();
	WindowWidth = Width;
	WindowHeight = Height;

	Renderer = std::make_unique<FRenderer>(Hwnd, Width, Height);
	if (!Renderer)
	{
		return false;
	}

	ObjManager = new ObjectManager();

	// Material
	FMaterialManager::Get().LoadAllMaterials(Renderer->GetDevice(), Renderer->GetRenderStateManager().get());

	// InputManager
	InputManager = new FInputManager();
	EnhancedInput = new FEnhancedInputManager();

	PhysicsManager = std::make_unique<FPhysicsManager>();

	// Timer
	Timer.Initialize();
	RegisterConsoleVariables();
	LevelManager = std::make_unique<FLevelManager>();
	const float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
	if (!LevelManager->Initialize(AspectRatio, StartupLevelType, Renderer.get()))
	{
		return false;
	}

	return true;
}



void FCore::SetViewportClient(FViewportClient* InViewportClient)
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

void FCore::ProcessInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
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

void FCore::Release()
{
	if (ViewportClient && Renderer)
	{
		ViewportClient->Detach(this, Renderer.get());
	}
	ViewportClient = nullptr;
	if (LevelManager)
	{
		LevelManager->Release();
		LevelManager.reset();
	}

	// Level 해제 후 PendingKill 오브젝트를 GC로 정리
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
		// 렌더러 Release는 소멸자에서 자동 호출
		Renderer.reset();
	}
}

void FCore::Tick()
{
	Timer.Tick();
	Tick(Timer.GetDeltaTime());
}

void FCore::Tick(const float DeltaTime)
{
	Input(DeltaTime);
	Physics(DeltaTime);
	GameLogic(DeltaTime);
	Render();
	LateUpdate(DeltaTime);
}

void FCore::Input(float DeltaTime)
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

void FCore::Physics(float DeltaTime)
{
	ULevel* Level = ViewportClient ? ViewportClient->ResolveLevel(this) : GetActiveLevel();
	
	if (Level)
	{
		FVector LineStart(2, 2, 0), LineEnd(5, 5, 0);
		FHitResult HitResult;

		bool bHit = PhysicsManager->Linetrace(Level, LineStart, LineEnd, HitResult);

		if (bHit)
		{
			if (!HitResult.HitActor->IsA(ASkySphereActor::StaticClass()))
			{
				for (UActorComponent* ActorComp : HitResult.HitActor->GetComponents())
				{

					if (!ActorComp->IsA(UPrimitiveComponent::StaticClass()))
					{
						continue;
					}
					//discard Billboard subUv
					UPrimitiveComponent* PrimComp = static_cast<UPrimitiveComponent*>(ActorComp);
					if (!PrimComp->ShouldDrawDebugBounds()) continue;

					FBoxSphereBounds Bound = PrimComp->GetWorldBounds();
					//DebugDrawManager를 통해 그림 → Flush()에서 일괄 렌더

					DebugDrawManager.DrawCube(Bound.Center, Bound.BoxExtent, FVector4(1, 0, 0, 1));

				}


				//Renderer->DrawCube(HitResult.HitLocation, FVector(0.1, 0.1, 0.1), FVector4(0, 1, 0, 1));
				//Renderer를 직접 호출 → DebugDrawManager를 거치지 않음
				DebugDrawManager.DrawCube(HitResult.HitLocation, FVector(0.1, 0.1, 0.1), FVector4(0, 1, 0, 1));
			}
		}

		if (Renderer)
		{
			DebugDrawManager.DrawLine(LineStart, LineEnd, FVector4(0, 1, 1, 1));
		}
	}
}

void FCore::GameLogic(float DeltaTime)
{
	UWorld* World = GetActiveWorld();
	if (World)
	{
		World->Tick(DeltaTime);
	}
}

void FCore::LateUpdate(float DeltaTime)
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

void FCore::Render()
{
	ULevel* Level = ViewportClient ? ViewportClient->ResolveLevel(this) : GetActiveLevel();
	if (!Renderer || !Level || Renderer->IsOccluded())
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
		ViewportClient->BuildRenderCommands(this, Level, Frustum, CommandQueue);
	}
	else
	{
		// Level->CollectRenderCommands(Frustum, CommandQueue);
	}

	Renderer->SubmitCommands(CommandQueue);
	Renderer->ExecuteCommands();
	const FShowFlags& ShowFlags = ViewportClient ? ViewportClient->GetShowFlags() : FShowFlags();
	DebugDrawManager.Flush(Renderer.get(), ShowFlags, ActiveWorld);
	Renderer->EndFrame();
}

void FCore::OnResize(int32 Width, int32 Height)
{
	if (Width == 0 || Height == 0) return;
	WindowWidth = Width;
	WindowHeight = Height;
	if (Renderer) Renderer->OnResize(Width, Height);
	if (LevelManager) LevelManager->OnResize(Width, Height);
}

void FCore::RegisterConsoleVariables()
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

