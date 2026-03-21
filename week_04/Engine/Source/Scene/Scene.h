#pragma once

#include "Object/Object.h"
#include "Object/ObjectFactory.h"
#include <d3d11.h>
#include "SceneTypes.h"
#include "ShowFlags.h"
class AActor;
class CCamera;
class FFrustum;
class UCameraComponent;
class UPrimitiveComponent;
struct FRenderCommandQueue;

class ENGINE_API UScene : public UObject
{
public:
	DECLARE_RTTI(UScene, UObject)
	~UScene();

	template <typename T>
	T* SpawnActor(const FString& InName)
	{
		static_assert(std::is_base_of_v<AActor, T>, "T must derive from AActor");

		T* NewActor = FObjectFactory::ConstructObject<T>(this, InName);
		if (!NewActor)
		{
			return nullptr;
		}
		RegisterActor(NewActor);
		NewActor->PostSpawnInitialize();

		return NewActor;
	}

	void RegisterActor(AActor* InActor);
	void DestroyActor(AActor* InActor);
	void CleanupDestroyedActors();

	const TArray<AActor*>& GetActors() const { return Actors; }
	void SetSceneType(ESceneType InSceneType) { SceneType = InSceneType; }
	ESceneType GetSceneType() const { return SceneType; }
	bool IsEditorScene() const { return SceneType == ESceneType::Editor; }
	bool IsGameScene() const { return SceneType == ESceneType::Game || SceneType == ESceneType::PIE; }

	void SetActiveCameraComponent(UCameraComponent* InCameraComponent);
	UCameraComponent* GetActiveCameraComponent() const;
	CCamera* GetCamera() const;

	void InitializeEmptyScene(float AspectRatio);
	void InitializeDefaultScene(float AspectRatio, ID3D11Device* Device = nullptr);
	void LoadSceneFromFile(const FString& FilePath, ID3D11Device* Device = nullptr);
	void SaveSceneToFile(const FString& FilePath);
	void ClearActors();
	void BeginPlay();
	void Tick(float DeltaTime);
	void CollectRenderCommands(const FFrustum& Frustum, FRenderCommandQueue& OutQueue);
	FShowFlags& GetShowFlags() { return ShowFlags; }
	const FShowFlags& GetShowFlags() const { return ShowFlags; }
private:
	void FrustrumCull(const FFrustum& Frustum, TArray<UPrimitiveComponent*>& OutVisible);

private:
	TArray<AActor*> Actors;
	UCameraComponent* SceneCameraComponent = nullptr;
	TObjectPtr<UCameraComponent> ActiveCameraComponent;
	bool bBegunPlay = false;
	ESceneType SceneType = ESceneType::Game;
	FShowFlags ShowFlags;
};
