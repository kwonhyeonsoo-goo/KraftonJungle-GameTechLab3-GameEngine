#include "LevelManager.h"
#include "World/Level.h"
#include "World/World.h"
#include "Object/ObjectFactory.h"
#include "Renderer/Renderer.h"
#include "Component/CameraComponent.h"

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

// ── World Context 생성/파괴 ────────────────────────────────────────────────

bool FLevelManager::CreateWorldContext(FWorldContext& OutContext, const FString& ContextName,
	ELevelType WorldType, float AspectRatio, bool bDefaultLevel)
{
	OutContext.ContextName = ContextName;
	OutContext.WorldType = WorldType;

	OutContext.World = FObjectFactory::ConstructObject<UWorld>(nullptr, ContextName);
	if (!OutContext.World) return false;

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

// ── Level 접근자 ───────────────────────────────────────────────────────────

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
	return (Context && Context->World) ? Context->World->GetLevel() : nullptr;
}

// ── World 전환 ─────────────────────────────────────────────────────────────

bool FLevelManager::ActivatePreviewLevel(const FString& ContextName)
{
	FEditorWorldContext* PreviewContext = FindPreviewWorld(ContextName);
	if (!PreviewContext) return false;
	ActiveWorldContext = PreviewContext;
	return true;
}

// ── 선택 Actor ─────────────────────────────────────────────────────────────

FEditorWorldContext* FLevelManager::GetActiveEditorContext()
{
	if (ActiveWorldContext == &EditorWorldContext)
		return &EditorWorldContext;

	for (const std::unique_ptr<FEditorWorldContext>& Context : PreviewWorldContexts)
	{
		if (Context && Context.get() == ActiveWorldContext)
			return Context.get();
	}
	return nullptr;
}

const FEditorWorldContext* FLevelManager::GetActiveEditorContext() const
{
	if (ActiveWorldContext == &EditorWorldContext)
		return &EditorWorldContext;

	for (const std::unique_ptr<FEditorWorldContext>& Context : PreviewWorldContexts)
	{
		if (Context && Context.get() == ActiveWorldContext)
			return Context.get();
	}
	return nullptr;
}

void FLevelManager::SetSelectedActor(AActor* InActor)
{
	FEditorWorldContext* Ctx = GetActiveEditorContext();
	if (Ctx) { Ctx->SelectedActor = InActor; return; }
	EditorWorldContext.SelectedActor = InActor;
}

AActor* FLevelManager::GetSelectedActor() const
{
	const FEditorWorldContext* Ctx = GetActiveEditorContext();
	return Ctx ? Ctx->SelectedActor : EditorWorldContext.SelectedActor;
}

// ── Preview ─────────────────────────────────────────────────────────────────

FEditorWorldContext* FLevelManager::FindPreviewWorld(const FString& ContextName)
{
	for (const std::unique_ptr<FEditorWorldContext>& Context : PreviewWorldContexts)
	{
		if (Context && Context->ContextName == ContextName)
			return Context.get();
	}
	return nullptr;
}

const FEditorWorldContext* FLevelManager::FindPreviewWorld(const FString& ContextName) const
{
	for (const std::unique_ptr<FEditorWorldContext>& Context : PreviewWorldContexts)
	{
		if (Context && Context->ContextName == ContextName)
			return Context.get();
	}
	return nullptr;
}

// public 래퍼 — FEditorEngine에서 호출
FEditorWorldContext* FLevelManager::FindPreviewWorldContext(const FString& ContextName)
{
	return FindPreviewWorld(ContextName);
}

const FEditorWorldContext* FLevelManager::FindPreviewWorldContext(const FString& ContextName) const
{
	return FindPreviewWorld(ContextName);
}

FEditorWorldContext* FLevelManager::CreatePreviewWorldContext(const FString& ContextName, int32 WindowWidth, int32 WindowHeight)
{
	if (ContextName.empty()) return nullptr;

	if (FEditorWorldContext* Existing = FindPreviewWorld(ContextName))
		return Existing;

	std::unique_ptr<FEditorWorldContext> PreviewContext = std::make_unique<FEditorWorldContext>();
	const float AspectRatio = (WindowHeight > 0)
		? (static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight))
		: 1.0f;

	if (!CreateWorldContext(*PreviewContext, ContextName, ELevelType::Preview, AspectRatio, false))
		return nullptr;

	FEditorWorldContext* Created = PreviewContext.get();
	PreviewWorldContexts.push_back(std::move(PreviewContext));
	return Created;
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
				if (!ActiveWorldContext) ActivateGameLevel();
			}
			DestroyWorldContext(*(*It));
			PreviewWorldContexts.erase(It);
			return true;
		}
	}
	return false;
}

// ── Resize ─────────────────────────────────────────────────────────────────
// AspectRatio 동기화는 ViewportClient::OnViewportResized() 책임
// LevelManager는 Renderer 리사이즈만 알림

void FLevelManager::OnResize(int32 Width, int32 Height)
{
	// 의도적으로 비워둠
	// 카메라 AspectRatio는 FViewport → SetViewportInfo() → OnViewportResized()로 처리됨
	(void)Width;
	(void)Height;
}