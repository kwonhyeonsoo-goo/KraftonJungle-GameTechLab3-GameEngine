#pragma once

#include "Object/Object.h"
#include "Object/ObjectFactory.h"
#include <d3d11.h>
#include "WorldType.h"
#include "Core/ShowFlags.h"
#include "World/RenderCollector.h"
class AActor;
class FCamera;
class FFrustum;
class UCameraComponent;
class UPrimitiveComponent;
struct FRenderCommandQueue;

class ENGINE_API ULevel : public UObject
{
public:
	DECLARE_RTTI(ULevel, UObject)
	DECLARE_DUPLICATE(ULevel)

	~ULevel();

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
	void SetLevelType(EWorldType InLevelType) { LevelType = InLevelType; }
	EWorldType GetLevelType() const { return LevelType; }
	bool IsEditorLevel() const { return LevelType == EWorldType::Editor; }
	bool IsGameLevel() const { return LevelType == EWorldType::Game || LevelType == EWorldType::PIE; }

  
	FCamera* GetCamera() const;


	void ClearActors();
	void BeginPlay();
	void Tick(float DeltaTime);

	void DuplicateSubObjects() override;

private:
	TArray<AActor*> Actors;
	bool bBegunPlay = false;
	EWorldType LevelType = EWorldType::Game;


};
