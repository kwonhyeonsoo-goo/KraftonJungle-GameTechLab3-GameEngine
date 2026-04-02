#include "LevelManager.h"
#include "World/Level.h"
#include "World/World.h"
#include "Object/ObjectFactory.h"
#include "Renderer/Renderer.h"
#include "Component/CameraComponent.h"
#include "Camera/Camera.h"

FLevelManager::~FLevelManager()
{
	Release();
}

bool FLevelManager::Initialize(float AspectRatio, ELevelType StartupLevelType, FRenderer* InRenderer)
{
	Renderer = InRenderer;

	FWorldContext* StartupContext = &GameWorldContext;
	FString ContextName = "GameLevel";

	if (StartupLevelType == ELevelType::Editor)
	{
		StartupContext = &EditorWorldContext;
		ContextName = "EditorLevel";
	}

	if (!CreateWorldContext(*StartupContext, ContextName, StartupLevelType, AspectRatio))
	{
		return false;
	}

	ActiveWorldContext = StartupContext;
	return true;
}

void FLevelManager::Release()
{
	ActiveWorldContext = nullptr;

	DestroyWorldContext(EditorWorldContext);
	DestroyWorldContext(GameWorldContext);
	Renderer = nullptr;
}

// ===== World Context 생성/파괴 =====

bool FLevelManager::CreateWorldContext(FWorldContext& OutContext, const FString& ContextName,
	ELevelType WorldType, float AspectRatio, bool bDefaultLevel)
{
	OutContext.ContextName = ContextName;
	OutContext.WorldType = WorldType;

	OutContext.World = FObjectFactory::ConstructObject<UWorld>(nullptr, ContextName);
	if (!OutContext.World)
	{
		return false;
	}

	OutContext.World->SetWorldType(WorldType);

	if (bDefaultLevel)
	{
		OutContext.World->InitializeWorld(AspectRatio, Renderer ? Renderer->GetDevice() : nullptr);
	}
	else
	{
		OutContext.World->InitializeWorld(AspectRatio);
	}

	return true;
}

void FLevelManager::DestroyWorldContext(FWorldContext& Context)
{
	if (Context.World)
	{
		Context.World->CleanupWorld();
		delete Context.World;
	}
	Context.Reset();
}

void FLevelManager::DestroyWorldContext(FEditorWorldContext& Context)
{
	if (Context.World)
	{
		Context.World->CleanupWorld();
		delete Context.World;
	}
	Context.Reset();
}

// ===== 하위 호환 Level 접근자 =====

ULevel* FLevelManager::GetActiveLevel() const
{
	UWorld* World = GetActiveWorld();
	return World ? World->GetLevel() : nullptr;
}

ULevel* FLevelManager::GetEditorLevel() const
{
	return EditorWorldContext.World ? EditorWorldContext.World->GetLevel() : nullptr;
}

ULevel* FLevelManager::GetGameLevel() const
{
	return GameWorldContext.World ? GameWorldContext.World->GetLevel() : nullptr;
}

// ===== Selected Actor =====

FEditorWorldContext* FLevelManager::GetActiveEditorContext()
{
	if (ActiveWorldContext == &EditorWorldContext)
	{
		return &EditorWorldContext;
	}

	return nullptr;
}

const FEditorWorldContext* FLevelManager::GetActiveEditorContext() const
{
	if (ActiveWorldContext == &EditorWorldContext)
	{
		return &EditorWorldContext;
	}

	return nullptr;
}

void FLevelManager::SetSelectedActor(AActor* InActor)
{
	FEditorWorldContext* ActiveEditorContext = GetActiveEditorContext();
	if (ActiveEditorContext)
	{
		ActiveEditorContext->SelectedActor = InActor;
		return;
	}

	EditorWorldContext.SelectedActor = InActor;
}

AActor* FLevelManager::GetSelectedActor() const
{
	const FEditorWorldContext* ActiveEditorContext = GetActiveEditorContext();
	if (ActiveEditorContext)
	{
		return ActiveEditorContext->SelectedActor.Get();
	}

	return EditorWorldContext.SelectedActor.Get();
}

// ===== Resize =====

void FLevelManager::OnResize(int32 Width, int32 Height)
{
	if (Width == 0 || Height == 0)
	{
		return;
	}

	const float NewAspect = static_cast<float>(Width) / static_cast<float>(Height);

	//auto UpdateAspect = [NewAspect](UWorld* World)
	//{
	//	if (World && World->GetCamera())
	//	{
	//		World->GetCamera()->SetAspectRatio(NewAspect);
	//	}
	//};

	//UpdateAspect(GameWorldContext.World);
	//UpdateAspect(EditorWorldContext.World);

}
