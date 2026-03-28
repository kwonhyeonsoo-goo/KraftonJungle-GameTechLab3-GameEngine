#pragma once
#include "CoreMinimal.h"
#include "World/WorldContext.h"
#include "World/LevelTypes.h"
#include <memory>

class ULevel;
class UWorld;
class AActor;
class CRenderer;

class ENGINE_API FLevelManager
{
public:
	FLevelManager() = default;
	~FLevelManager();
	FLevelManager(const FLevelManager&) = delete;
	FLevelManager& operator=(const FLevelManager&) = delete;
	FLevelManager(FLevelManager&&) = delete;
	FLevelManager& operator=(FLevelManager&&) = delete;

	// 초기화
	bool Initialize(float AspectRatio, ELevelType StartupLevelType, CRenderer* InRenderer);
	void Release();

	// World 전환
	void ActivateEditorLevel() { ActiveWorldContext = EditorWorldContext.World ? &EditorWorldContext : nullptr; }
	void ActivateGameLevel() { ActiveWorldContext = GameWorldContext.World ? &GameWorldContext : nullptr; }
	bool ActivatePreviewLevel(const FString& ContextName);

	// Preview 관리
	FEditorWorldContext* CreatePreviewWorldContext(const FString& ContextName, int32 WindowWidth, int32 WindowHeight);
	bool DestroyPreviewWorld(const FString& ContextName);

	// World 접근자
	UWorld* GetActiveWorld() const { return ActiveWorldContext ? ActiveWorldContext->World : nullptr; }
	UWorld* GetEditorWorld() const { return EditorWorldContext.World; }
	UWorld* GetGameWorld() const { return GameWorldContext.World; }

	const FWorldContext* GetActiveWorldContext() const { return ActiveWorldContext; }
	const TArray<std::unique_ptr<FEditorWorldContext>>& GetPreviewWorldContexts() const { return PreviewWorldContexts; }

	// 하위 호환 — World 경유로 Level 반환
	ULevel* GetActiveLevel() const;
	ULevel* GetEditorLevel() const;
	ULevel* GetGameLevel() const;
	ULevel* GetPreviewLevel(const FString& ContextName) const;

	// 선택 Actor
	void SetSelectedActor(AActor* InActor);
	AActor* GetSelectedActor() const;

	// Resize
	void OnResize(int32 Width, int32 Height);

private:
	bool CreateWorldContext(FWorldContext& OutContext, const FString& ContextName,
		ELevelType WorldType, float AspectRatio, bool bDefaultLevel = true);
	void DestroyWorldContext(FWorldContext& Context);
	void DestroyWorldContext(FEditorWorldContext& Context);

	FEditorWorldContext* GetActiveEditorContext();
	const FEditorWorldContext* GetActiveEditorContext() const;
	FEditorWorldContext* FindPreviewWorld(const FString& ContextName);
	const FEditorWorldContext* FindPreviewWorld(const FString& ContextName) const;

private:
	FWorldContext GameWorldContext;
	FEditorWorldContext EditorWorldContext;
	TArray<std::unique_ptr<FEditorWorldContext>> PreviewWorldContexts;
	FWorldContext* ActiveWorldContext = nullptr;
	CRenderer* Renderer = nullptr;
};
