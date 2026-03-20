#pragma once
#include "CoreMinimal.h"
#include "Scene/SceneContext.h"
#include "Scene/SceneTypes.h"
#include <memory>

class UScene;
class AActor;
class CRenderer;

class ENGINE_API FSceneManager
{
public:
	FSceneManager() = default;
	~FSceneManager();

	// 초기화
	bool Initialize(float AspectRatio, ESceneType StartupSceneType, CRenderer* InRenderer);
	void Release();

	// Scene 전환
	void ActivateEditorScene() { ActiveSceneContext = EditorSceneContext.Scene ? &EditorSceneContext : nullptr; }
	void ActivateGameScene() { ActiveSceneContext = GameSceneContext.Scene ? &GameSceneContext : nullptr; }
	bool ActivatePreviewScene(const FString& ContextName);

	// Preview Scene 관리
	FEditorSceneContext* CreatePreviewScene(const FString& ContextName, int32 WindowWidth, int32 WindowHeight);
	bool DestroyPreviewScene(const FString& ContextName);

	// 접근자
	UScene* GetActiveScene() const { return ActiveSceneContext ? ActiveSceneContext->Scene : nullptr; }
	UScene* GetEditorScene() const { return EditorSceneContext.Scene; }
	UScene* GetGameScene() const { return GameSceneContext.Scene; }
	UScene* GetPreviewScene(const FString& ContextName) const;

	const FSceneContext* GetActiveSceneContext() const { return ActiveSceneContext; }
	const TArray<std::unique_ptr<FEditorSceneContext>>& GetPreviewSceneContexts() const { return PreviewSceneContexts; }

	// 선택 Actor
	void SetSelectedActor(AActor* InActor);
	AActor* GetSelectedActor() const;

	// Resize 시 AspectRatio 갱신
	void OnResize(int32 Width, int32 Height);

private:
	bool CreateSceneContext(FSceneContext& OutContext, const FString& ContextName,
		ESceneType SceneType, float AspectRatio, bool bDefaultScene = true);
	void DestroySceneContext(FSceneContext& Context);
	void DestroySceneContext(FEditorSceneContext& Context);

	FEditorSceneContext* GetActiveEditorContext();
	const FEditorSceneContext* GetActiveEditorContext() const;
	FEditorSceneContext* FindPreviewScene(const FString& ContextName);
	const FEditorSceneContext* FindPreviewScene(const FString& ContextName) const;

private:
	FSceneContext GameSceneContext;
	FEditorSceneContext EditorSceneContext;
	TArray<std::unique_ptr<FEditorSceneContext>> PreviewSceneContexts;
	FSceneContext* ActiveSceneContext = nullptr;
	CRenderer* Renderer = nullptr;  // 비소유 포인터 (Scene 생성 시 Device 필요)
};
