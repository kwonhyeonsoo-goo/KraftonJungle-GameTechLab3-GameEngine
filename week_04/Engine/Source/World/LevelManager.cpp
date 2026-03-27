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

bool FLevelManager::Initialize(float AspectRatio, ELevelType StartupLevelType, CRenderer* InRenderer)
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

	for (std::unique_ptr<FEditorWorldContext>& PreviewContext : PreviewWorldContexts)
	{
		if (PreviewContext)
		{
			DestroyWorldContext(*PreviewContext);
		}
	}
	PreviewWorldContexts.clear();

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

ULevel* FLevelManager::GetPreviewLevel(const FString& ContextName) const
{
	const FEditorWorldContext* Context = FindPreviewWorld(ContextName);
	if (Context && Context->World)
	{
		return Context->World->GetLevel();
	}
	return nullptr;
}

// ===== Activate =====

bool FLevelManager::ActivatePreviewLevel(const FString& ContextName)
{
	FEditorWorldContext* PreviewContext = FindPreviewWorld(ContextName);
	if (PreviewContext == nullptr)
	{
		return false;
	}

	ActiveWorldContext = PreviewContext;
	return true;
}

// ===== Selected Actor =====

FEditorWorldContext* FLevelManager::GetActiveEditorContext()
{
	if (ActiveWorldContext == &EditorWorldContext)
	{
		return &EditorWorldContext;
	}

	for (const std::unique_ptr<FEditorWorldContext>& Context : PreviewWorldContexts)
	{
		if (Context && Context.get() == ActiveWorldContext)
		{
			return Context.get();
		}
	}

	return nullptr;
}

const FEditorWorldContext* FLevelManager::GetActiveEditorContext() const
{
	if (ActiveWorldContext == &EditorWorldContext)
	{
		return &EditorWorldContext;
	}

	for (const std::unique_ptr<FEditorWorldContext>& Context : PreviewWorldContexts)
	{
		if (Context && Context.get() == ActiveWorldContext)
		{
			return Context.get();
		}
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
		return ActiveEditorContext->SelectedActor;
	}

	return EditorWorldContext.SelectedActor;
}

// ===== Preview =====

FEditorWorldContext* FLevelManager::FindPreviewWorld(const FString& ContextName)
{
	for (const std::unique_ptr<FEditorWorldContext>& Context : PreviewWorldContexts)
	{
		if (Context && Context->ContextName == ContextName)
		{
			return Context.get();
		}
	}
	return nullptr;
}

const FEditorWorldContext* FLevelManager::FindPreviewWorld(const FString& ContextName) const
{
	for (const std::unique_ptr<FEditorWorldContext>& Context : PreviewWorldContexts)
	{
		if (Context && Context->ContextName == ContextName)
		{
			return Context.get();
		}
	}
	return nullptr;
}

FEditorWorldContext* FLevelManager::CreatePreviewWorldContext(const FString& ContextName, int32 WindowWidth, int32 WindowHeight)
{
	if (ContextName.empty())
	{
		return nullptr;
	}

	if (FEditorWorldContext* ExistingContext = FindPreviewWorld(ContextName))
	{
		return ExistingContext;
	}

	std::unique_ptr<FEditorWorldContext> PreviewContext = std::make_unique<FEditorWorldContext>();
	const float AspectRatio = (WindowHeight > 0)
		? (static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight))
		: 1.0f;

	if (!CreateWorldContext(*PreviewContext, ContextName, ELevelType::Preview, AspectRatio, false))
	{
		return nullptr;
	}

	FEditorWorldContext* CreatedContext = PreviewContext.get();
	PreviewWorldContexts.push_back(std::move(PreviewContext));
	return CreatedContext;
}

bool FLevelManager::DestroyPreviewWorld(const FString& ContextName)
{
	for (auto It = PreviewWorldContexts.begin(); It != PreviewWorldContexts.end(); ++It)
	{
		if (*It && (*It)->ContextName == ContextName)
		{
			if (ActiveWorldContext == It->get())
			{
				ActivateEditorLevel();
				if (ActiveWorldContext == nullptr)
				{
					ActivateGameLevel();
				}
			}

			DestroyWorldContext(*(*It));
			PreviewWorldContexts.erase(It);
			return true;
		}
	}

	return false;
}

// ===== Resize =====

void FLevelManager::OnResize(int32 Width, int32 Height)
{
	if (Width == 0 || Height == 0)
	{
		return;
	}

	const float NewAspect = static_cast<float>(Width) / static_cast<float>(Height);

	auto UpdateAspect = [NewAspect](UWorld* World)
	{
		if (World && World->GetCamera())
		{
			World->GetCamera()->SetAspectRatio(NewAspect);
		}
	};

	UpdateAspect(GameWorldContext.World);
	UpdateAspect(EditorWorldContext.World);

	for (const std::unique_ptr<FEditorWorldContext>& PreviewContext : PreviewWorldContexts)
	{
		if (PreviewContext)
		{
			UpdateAspect(PreviewContext->World);
		}
	}
}
