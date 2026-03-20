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
#include "Math/Frustum.h"

CCore::~CCore()
{
	Release();
}

bool CCore::CreateSceneContext(FSceneContext& Context, const FString& ContextName, ESceneType SceneType, float AspectRatio, bool bInitializeDefaultScene)
{
	Context.ContextName = ContextName;
	Context.SceneType = SceneType;
	Context.Scene = FObjectFactory::ConstructObject<UScene>(nullptr, ContextName);
	if (!Context.Scene)
	{
		return false;
	}

	Context.Scene->SetSceneType(SceneType);
	if (bInitializeDefaultScene)
	{
		Context.Scene->InitializeDefaultScene(AspectRatio, Renderer ? Renderer->GetDevice() : nullptr);
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
	FPaths::Initialize();
	WindowWidth = Width;
	WindowHeight = Height;

	Renderer = std::make_unique<CRenderer>();
	if (!Renderer->Initialize(Hwnd, Width, Height))
	{
		return false;
	}

	ObjManager = new ObjectManager();

	// InputManager
	InputManager = new CInputManager();
	EnhancedInput = new CEnhancedInputManager();
	// Timer
	Timer.Initialize();
	RegisterConsoleVariables();

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

	const TArray<AActor*>& Actors = Scene->GetActors();

	FVector LineStart(0, 0, 0), LineEnd(1, 1, 0);
	FVector LineDirection = LineEnd - LineStart;
	LineDirection.Normalize();

	// Normalize 실패 상황 고려
	if (!LineDirection.IsZero())
	{
		// Culling 고려
		for (AActor* Actor : Actors)
		{
			if (!Actor || Actor->IsPendingDestroy())
			{
				continue;
			}

			for (UActorComponent* Component : Actor->GetComponents())
			{
				if (!Component->IsA(UPrimitiveComponent::StaticClass()))
				{
					continue;
				}

				UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
				if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData())
				{
					continue;
				}

				FBoxSphereBounds Bound = PrimitiveComponent->GetWorldBoundsForAABB();

				if (Renderer)
				{
					Renderer->DrawLine(LineStart, LineEnd, FVector4(0, 1, 1, 1));

					Renderer->DrawCube(Bound.Center, Bound.BoxExtent, FVector4(1, 0, 0, 1));
				}

				FVector VecToOrigin = Bound.Center - LineStart;
				float ShortestT = FVector::DotProduct(VecToOrigin, LineDirection);
				FVector ShortestPos = LineStart + LineDirection * ShortestT;
				float ShortestDistSquared = (ShortestPos - Bound.Center).SizeSquared();
				
				// 빠른 검사를 위해 일차적으로 Sphere 로 test
				if (ShortestDistSquared <= Bound.RadiusSquared)
				{
					// 정밀한 검사를 위해 Box test
					FVector SlabMin = Bound.Center - Bound.BoxExtent;
					FVector SlabMax = Bound.Center + Bound.BoxExtent;

					FVector DirectionInv(1 / LineDirection.X, 1 / LineDirection.Y, 1 / LineDirection.Z);

					FVector T1 = FVector::Multiply((SlabMin - LineStart), DirectionInv);
					FVector T2 = FVector::Multiply((SlabMax - LineStart), DirectionInv);

					FVector TMinVec = FVector::Min(T1, T2);
					FVector TMaxVec = FVector::Max(T1, T2);

					float tNear = FMath::Max(FMath::Max(TMinVec.X, TMinVec.Y), TMinVec.Z);
					float tFar = FMath::Min(FMath::Min(TMaxVec.X, TMaxVec.Y), TMaxVec.Z);

					bool bIntersected =
						(tNear <= tFar) &&
						(tFar >= 0.0f) &&
						(tNear * tNear <= (LineEnd - LineStart).SizeSquared());

					if (bIntersected)
					{
						UE_LOG("Line Collided");
					}
				}
				else
				{
					// rejct
				}
				
			}
		}


		// AABB 도 그려야하는데

	}
}

void CCore::GameLogic(float DeltaTime)
{
	UScene* Scene = GetActiveScene();
	if (Scene)
	{
		Scene->Tick(DeltaTime);
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

	UCameraComponent* ActiveCamera = Scene->GetActiveCameraComponent();
	if (!ActiveCamera)
	{
		Renderer->EndFrame();
		return;
	}

	FRenderCommandQueue CommandQueue;
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
		Scene->CollectRenderCommands(Frustum, CommandQueue);
	}

	Renderer->SubmitCommands(CommandQueue);
	Renderer->ExecuteCommands();
	Renderer->EndFrame();
}

void CCore::OnResize(int32 Width, int32 Height)
{
	if (Width == 0 || Height == 0)
	{
		return;
	}

	WindowWidth = Width;
	WindowHeight = Height;

	if (Renderer)
	{
		Renderer->OnResize(Width, Height);
	}

	const float NewAspect = static_cast<float>(Width) / static_cast<float>(Height);
	auto UpdateSceneAspectRatio = [NewAspect](UScene* Scene)
		{
			if (Scene && Scene->GetCamera())
			{
				Scene->GetCamera()->SetAspectRatio(NewAspect);
			}
		};

	UpdateSceneAspectRatio(GameSceneContext.Scene);
	UpdateSceneAspectRatio(EditorSceneContext.Scene);
	for (const std::unique_ptr<FEditorSceneContext>& PreviewContext : PreviewSceneContexts)
	{
		if (PreviewContext)
		{
			UpdateSceneAspectRatio(PreviewContext->Scene);
		}
	}
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
