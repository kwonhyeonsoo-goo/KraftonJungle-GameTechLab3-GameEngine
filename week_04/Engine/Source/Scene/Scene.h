#pragma once
#include "Object/Object.h"
#include "Object/Class.h"
#include "Scene/SceneTypes.h"

class AActor;
class CCamera;
class ENGINE_API UScene : public UObject
{
public:
	static UClass* StaticClass();

	UScene(UClass* InClass, const FString& InName, UObject* InOuter = nullptr);
	~UScene() override;

	template <typename T>
	T* SpawnActor(const FString& InName)
	{
		static_assert(std::is_base_of_v<AActor, T>, "T must derive from AActor");

		UObject* NewObj = T::StaticClass()->CreateInstance(this, InName);
		if (!NewObj)
		{
			return nullptr;
		}

		T* NewActor = static_cast<T*>(NewObj);
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

	CCamera* GetCamera() const { return Camera; }

	void InitializeEmptyScene(float AspectRatio);
	void InitializeDefaultScene(float AspectRatio);
	void LoadSceneFromFile(const FString& FilePath);
	void SaveSceneToFile(const FString& FilePath);
	void ClearActors();
	void BeginPlay();
	void Tick(float DeltaTime);

private:
	TArray<AActor*> Actors;
	CCamera* Camera = nullptr;
	bool bBegunPlay = false;
	ESceneType SceneType = ESceneType::Game;
};
