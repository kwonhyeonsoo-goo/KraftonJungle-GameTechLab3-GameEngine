#include "SceneManager.h"
#include "Scene/Scene.h"
#include "Renderer/Renderer.h"
#include "Component/CameraComponent.h"
FSceneManager::~FSceneManager()
{
	Release();

}

bool FSceneManager::Initialize(float AspectRatio, ESceneType StartupSceneType, CRenderer* InRenderer)
{
	Renderer = InRenderer;

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

void FSceneManager::Release()
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
	Renderer = nullptr;
}

bool FSceneManager::CreateSceneContext(FSceneContext& OutContext, const FString& ContextName, ESceneType SceneType, float AspectRatio, bool bDefaultScene)
{
	OutContext.ContextName = ContextName;
	OutContext.SceneType = SceneType;
	OutContext.Scene = FObjectFactory::ConstructObject<UScene>(nullptr, ContextName);
	if (!OutContext.Scene)
	{
		return false;
	}

	OutContext.Scene->SetSceneType(SceneType);
	if (bDefaultScene)
	{
		OutContext.Scene->InitializeDefaultScene(AspectRatio, Renderer ? Renderer->GetDevice() : nullptr);
	}
	else
	{
		OutContext.Scene->InitializeEmptyScene(AspectRatio);
	}

	return true;
}

void FSceneManager::DestroySceneContext(FSceneContext& Context)
{
	delete Context.Scene;
	Context.Reset();
}

void FSceneManager::DestroySceneContext(FEditorSceneContext& Context)
{
	delete Context.Scene;
	Context.Reset();
}

FEditorSceneContext* FSceneManager::GetActiveEditorContext()
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

const FEditorSceneContext* FSceneManager::GetActiveEditorContext() const
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

FEditorSceneContext* FSceneManager::FindPreviewScene(const FString& ContextName)
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

const FEditorSceneContext* FSceneManager::FindPreviewScene(const FString& ContextName) const
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


bool FSceneManager::ActivatePreviewScene(const FString& ContextName)
{
	FEditorSceneContext* PreviewContext = FindPreviewScene(ContextName);
	if (PreviewContext == nullptr)
	{
		return false;
	}

	ActiveSceneContext = PreviewContext;
	return true;
}



UScene* FSceneManager::GetPreviewScene(const FString& ContextName) const
{
	const FEditorSceneContext* Context = FindPreviewScene(ContextName);
	return Context ? Context->Scene : nullptr;
}

void FSceneManager::SetSelectedActor(AActor* InActor)
{
	FEditorSceneContext* ActiveEditorContext = GetActiveEditorContext();
	if (ActiveEditorContext)
	{
		ActiveEditorContext->SelectedActor = InActor;
		return;
	}

	EditorSceneContext.SelectedActor = InActor;
}
AActor* FSceneManager::GetSelectedActor() const
{
	const FEditorSceneContext* ActiveEditorContext = GetActiveEditorContext();
	if (ActiveEditorContext)
	{
		return ActiveEditorContext->SelectedActor;
	}

	return EditorSceneContext.SelectedActor;
}
void FSceneManager::OnResize(int32 Width, int32 Height)
{
	if (Width == 0 || Height == 0)
	{
		return;
	}
	const float NewAspect = static_cast<float>(Width) / static_cast<float>(Height);
	auto UpdateAspect = [NewAspect](UScene* Scene)
	{
		if (Scene && Scene->GetCamera())
		{
			Scene->GetCamera()->SetAspectRatio(NewAspect);
		}
	};



	UpdateAspect(GameSceneContext.Scene);
	UpdateAspect(EditorSceneContext.Scene);
	for (const std::unique_ptr<FEditorSceneContext>& PreviewContext : PreviewSceneContexts)
	{
		if (PreviewContext)
		{
			UpdateAspect(PreviewContext->Scene);
		}
	}
}
FEditorSceneContext* FSceneManager::CreatePreviewScene(const FString& ContextName, int32 WindowWidth, int32 WindowHeight)
{
	if (ContextName.empty())
	{
		return nullptr;
	}

	if (FEditorSceneContext* ExistingContext = FindPreviewScene(ContextName))
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

bool FSceneManager::DestroyPreviewScene(const FString& ContextName)
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

