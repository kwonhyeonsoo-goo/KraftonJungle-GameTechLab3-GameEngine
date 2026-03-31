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
#include "Debug/EngineLog.h"

FCore::~FCore()
{
	Release();
}

bool FCore::Initialize(HWND Hwnd, int32 Width, int32 Height, ELevelType StartupLevelType)
{
	FPaths::Initialize();
	WindowWidth = Width;
	WindowHeight = Height;

	FrameRenderParams.UVScrollVelocity[1] = 0.5f;

	Renderer = std::make_unique<FRenderer>(Hwnd, Width, Height);
	if (!Renderer)
	{
		return false;
	}

	ObjManager = new ObjectManager();

	FMaterialManager::Get().LoadAllMaterials(Renderer->GetDevice(), Renderer->GetRenderStateManager().get());

	InputManager = new FInputManager();
	EnhancedInput = new FEnhancedInputManager();

	PhysicsManager = std::make_unique<FPhysicsManager>();

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

// ── ViewportClient 배열 등록 ───────────────────────────────────────────────
// 소유권은 FEngine(unique_ptr), Core는 raw pointer만 보관

void FCore::SetViewportClients(TArray<std::unique_ptr<IViewportClient>>& InArray)
{
	// 기존 포인터 전부 Detach
	for (IViewportClient* VP : ViewportClientArray)
	{
		if (VP && Renderer)
		{
			VP->Detach(this, Renderer.get());
		}
	}
	ViewportClientArray.clear();

	// 새 포인터 등록 후 Attach
	for (auto& VP : InArray)
	{
		if (!VP) continue;
		ViewportClientArray.push_back(VP.get());
		VP->Attach(this, Renderer.get());
	}
}

IViewportClient* FCore::GetViewportClientAt(int32 Index) const
{
	if (Index < 0 || Index >= static_cast<int32>(ViewportClientArray.size()))
	{
		return nullptr;
	}
	return ViewportClientArray[Index];
}

// ── 입력 ──────────────────────────────────────────────────────────────────

void FCore::ProcessInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (InputManager)
	{
		InputManager->ProcessMessage(Hwnd, Msg, WParam, LParam);
	}

	// TODO: 포커스된 뷰포트만 입력 수신하도록 개선 필요
	for (IViewportClient* VP : ViewportClientArray)
	{
		if (VP)
		{
			VP->HandleMessage(this, Hwnd, Msg, WParam, LParam);
		}
	}
}

// ── 해제 ──────────────────────────────────────────────────────────────────

void FCore::Release()
{
	for (IViewportClient* VP : ViewportClientArray)
	{
		if (VP && Renderer)
		{
			VP->Detach(this, Renderer.get());
		}
	}
	ViewportClientArray.clear();

	if (LevelManager)
	{
		LevelManager->Release();
		LevelManager.reset();
	}

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
		Renderer.reset();
	}
}

// ── Tick ──────────────────────────────────────────────────────────────────

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

	// TODO: 포커스된 뷰포트만 Tick 받도록 개선 필요
	for (IViewportClient* VP : ViewportClientArray)
	{
		if (VP)
		{
			VP->Tick(this, DeltaTime);
		}
	}
}

void FCore::Physics(float DeltaTime)
{
	ULevel* Level = GetActiveLevel();
	if (!Level) return;

	FVector LineStart(2, 2, 0), LineEnd(5, 5, 0);
	FHitResult HitResult;

	bool bHit = PhysicsManager->Linetrace(Level, LineStart, LineEnd, HitResult);
	if (bHit && !HitResult.HitActor->IsA(ASkySphereActor::StaticClass()))
	{
		for (UActorComponent* ActorComp : HitResult.HitActor->GetComponents())
		{
			if (!ActorComp->IsA(UPrimitiveComponent::StaticClass())) continue;
			UPrimitiveComponent* PrimComp = static_cast<UPrimitiveComponent*>(ActorComp);
			if (!PrimComp->ShouldDrawDebugBounds()) continue;
			FBoxSphereBounds Bound = PrimComp->GetWorldBounds();
			DebugDrawManager.DrawCube(Bound.Center, Bound.BoxExtent, FVector4(1, 0, 0, 1));
		}
		DebugDrawManager.DrawCube(HitResult.HitLocation, FVector(0.1f, 0.1f, 0.1f), FVector4(0, 1, 0, 1));
	}

	if (Renderer)
	{
		DebugDrawManager.DrawLine(LineStart, LineEnd, FVector4(0, 1, 1, 1));
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
	if (GCInterval <= 0.0) return;

	double CurrentTime = Timer.GetTotalTime();
	if (ObjManager && (CurrentTime - LastGCTime) >= GCInterval)
	{
		ObjManager->FlushKilledObjects();
		LastGCTime = CurrentTime;
	}
}

// ── 렌더 ──────────────────────────────────────────────────────────────────

void FCore::Render()
{
	if (!Renderer || Renderer->IsOccluded()) return;

	// 폴백용 ActiveLevel — LinkedWorld가 없는 VP에서 사용
	ULevel* FallbackLevel = GetActiveLevel();
	UWorld* FallbackWorld = GetActiveWorld();

	if (!FallbackLevel)
	{
		return;
	}

	FrameRenderParams.Time = Timer.GetTotalTime();
	Renderer->BeginFrame(FrameRenderParams);

	if (!FallbackWorld)
	{
		Renderer->EndFrame();
		return;
	}

	// ── SkySphere 카메라 위치 주입 ─────────────────────────────────────
	// VP[0]의 LinkedWorld 또는 FallbackWorld 기준
	// VP[0]의 카메라 위치로 SkySphere 이동
	//if (!ViewportClientArray.empty() && ViewportClientArray[0])
	//{
	//	IViewportClient* PrimaryVP = ViewportClientArray[0];
	//	UWorld* PrimaryWorld = PrimaryVP->GetLinkedWorld()
	//		? PrimaryVP->GetLinkedWorld()
	//		: FallbackWorld;

	//	const FCameraViewInfo PrimaryViewInfo = PrimaryVP->GetCameraViewInfo();
	//	if (ULevel* PrimaryLevel = PrimaryWorld->GetLevel())
	//	{
	//		for (AActor* Actor : PrimaryLevel->GetActors())
	//		{
	//			if (ASkySphereActor* Sky = dynamic_cast<ASkySphereActor*>(Actor))
	//			{
	//				Sky->SetCameraPosition(PrimaryViewInfo.Position);
	//			}
	//		}
	//	}
	//}

	for (IViewportClient* CurrentViewport : ViewportClientArray)
	{
		if (!CurrentViewport) continue;

		Renderer->ClearCommandList();

		// ── 0. VP의 LinkedWorld 우선, 없으면 FallbackLevel 사용 ────
		// 방향 B: 모든 VP가 동일한 World를 보되 카메라만 다름
		ULevel* Level = CurrentViewport->GetLinkedWorld()
			? CurrentViewport->GetLinkedWorld()->GetLevel()
			: FallbackLevel;

		UWorld* CurrentWorld = CurrentViewport->GetLinkedWorld()
			? CurrentViewport->GetLinkedWorld()
			: FallbackWorld;

		if (!Level) continue;

		// ── 1. RTV/DSV + D3D11_VIEWPORT 세팅 ──────────────────────
		const FViewportInfo& VPInfo = CurrentViewport->GetViewportInfo();
		if (VPInfo.Width <= 0.f || VPInfo.Height <= 0.f) continue;
		if (!VPInfo.RTV || !VPInfo.DSV) continue;

		D3D11_VIEWPORT D3DViewport = {};
		D3DViewport.TopLeftX = VPInfo.TopLeftX;
		D3DViewport.TopLeftY = VPInfo.TopLeftY;
		D3DViewport.Width = VPInfo.Width;
		D3DViewport.Height = VPInfo.Height;
		D3DViewport.MinDepth = VPInfo.MinDepth;
		D3DViewport.MaxDepth = VPInfo.MaxDepth;

		Renderer->SetLevelRenderTarget(VPInfo.RTV, VPInfo.DSV, D3DViewport);

		// ── 2. 카메라 / 행렬 ───────────────────────────────────────
		const FCameraViewInfo CameraViewInfo = CurrentViewport->GetCameraViewInfo();
		CommandQueue.Clear();
		CommandQueue.Reserve(Renderer->GetPrevCommandCount());
		CommandQueue.ViewMatrix = CameraViewInfo.ViewMatrix;
		CommandQueue.ProjectionMatrix = CameraViewInfo.ProjectionMatrix;

		// ── 3. Frustum / 렌더 커맨드 ──────────────────────────────
		FFrustum Frustum;
		Frustum.ExtractFromVP(CameraViewInfo.ViewMatrix * CameraViewInfo.ProjectionMatrix);

		CurrentViewport->BuildRenderCommands(this, Level, Frustum, CommandQueue);
		Renderer->SubmitCommands(CommandQueue);
		Renderer->ExecuteCommands();

		// ── 4. Debug Draw ──────────────────────────────────────────
		const FShowFlags& ShowFlags = CurrentViewport->GetShowFlags();
		DebugDrawManager.Flush(Renderer.get(), ShowFlags, CurrentWorld);
		Renderer->ClearLevelRenderTarget();
	}

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

	FConsoleVariable* StatVar = CVM.Find("t.MaxFPS");
	if (!StatVar)
	{
		StatVar = CVM.Register("stat", "", "Show Stat On Last Focused Viewport");
	}
	
	StatVar->SetOnChanged([this](FConsoleVariable* Var) {  
		FString VarString = Var->GetString();

		if (LastFocusedVP)
		{
			if (VarString == "fps")
			{
				// 임의 테스트
				FShowFlags ShowFlags = LastFocusedVP->GetShowFlags();
				if (ShowFlags.HasFlag(EEngineShowFlags::SF_StatFPS))
				{
					LastFocusedVP->GetShowFlags().SetFlag(EEngineShowFlags::SF_StatFPS, false);
				}
				else
				{
					LastFocusedVP->GetShowFlags().SetFlag(EEngineShowFlags::SF_StatFPS, true);
				}
			}
			else if (VarString == "memory")
			{
				// 임의 테스트
				FShowFlags ShowFlags = LastFocusedVP->GetShowFlags();
				if (ShowFlags.HasFlag(EEngineShowFlags::SF_StatMemory))
				{
					LastFocusedVP->GetShowFlags().SetFlag(EEngineShowFlags::SF_StatMemory, false);
				}
				else
				{
					LastFocusedVP->GetShowFlags().SetFlag(EEngineShowFlags::SF_StatMemory, true);
				}
			}
			else if (VarString == "none")
			{
				for (IViewportClient* VP : ViewportClientArray)
				{
					VP->GetShowFlags().SetFlag(EEngineShowFlags::SF_StatFPS, false);
					VP->GetShowFlags().SetFlag(EEngineShowFlags::SF_StatMemory, false);
				}
			}
		}
		
	});

	FConsoleVariable* MaxFPSVar = CVM.Find("t.MaxFPS");
	if (!MaxFPSVar)
	{
		MaxFPSVar = CVM.Register("t.MaxFPS", 0.0f, "Maximum FPS limit (0 = unlimited)");
	}
	MaxFPSVar->SetOnChanged([this](FConsoleVariable* Var) { Timer.SetMaxFPS(Var->GetFloat()); });
	Timer.SetMaxFPS(MaxFPSVar->GetFloat());

	FConsoleVariable* VSyncVar = CVM.Find("r.VSync");
	if (!VSyncVar)
	{
		VSyncVar = CVM.Register("r.VSync", 0, "Enable VSync (0 = off, 1 = on)");
	}
	VSyncVar->SetOnChanged([this](FConsoleVariable* Var)
		{
			if (Renderer) Renderer->SetVSync(Var->GetInt() != 0);
		});
	if (Renderer) Renderer->SetVSync(VSyncVar->GetInt() != 0);

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