#pragma once
#include "Object/Object.h"

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

	CCamera* GetCamera() const { return Camera; }

	void InitializeDefaultScene(float AspectRatio);
	void LoadSceneFromFile(const FString& FilePath);
	void BeginPlay();
	void Tick(float DeltaTime);

private:
	TArray<AActor*> Actors;
	CCamera* Camera = nullptr;
	bool bBegunPlay = false;
};
