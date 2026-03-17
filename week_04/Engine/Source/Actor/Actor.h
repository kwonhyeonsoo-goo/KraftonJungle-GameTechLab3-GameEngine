#pragma once
#include "Component/SceneComponent.h"
#include "Object/Object.h"

class UActorComponent;
class USceneComponent;
class UScene;

class ENGINE_API AActor : public UObject
{
public:
	static UClass* StaticClass();

	AActor(UClass* InClass, const FString& InName, UObject* InOuter = nullptr);
	~AActor() override = default;

	UScene* GetScene() const { return Scene; }
	void SetScene(UScene* InScene) { Scene = InScene; }

	// ULevel* GetLevel() const { return Level;
	// void SetLevel(ULevel* InLevel) { Level = InLevel; }

	USceneComponent* GetRootComponent() const { return RootComponent; }
	void SetRootComponent(USceneComponent* InRootComponent)
	{
		// 의문점
		// 기존에 RootComponent가 있을 시에는 RootComponent의 OwnerActor를 지워주나?
		// 이러면 두 개의 RootComponent가 하나의 Owner을 가지고 있는건데.
		RootComponent = InRootComponent;
		if (RootComponent)
		{
			RootComponent->SetOwner(this);
		}
	}
	const TArray<UActorComponent*>& GetComponents() const { return OwnedComponents; }
	void AddOwnedComponent(UActorComponent* InComponent);
	void RemoveOwnedComponent(UActorComponent* InComponent);

	template <typename T>
	T* GetComponentByClass() const
	{
		for (UActorComponent* Component : OwnedComponents)
		{
			if (Component && Component->IsA(T::StaticClass()))
			{
				return static_cast<T*>(Component);
			}
		}
		return nullptr;
	}

	virtual void PostSpawnInitialize();
	virtual void BeginPlay();
	virtual void Tick(float DeltaTime);
	virtual void EndPlay();
	virtual void Destroy();

	bool HasBegunPlay() const { return bActorBegunPlay; }
	bool IsPendingDestroy() const { return bPendingDestroy; }
	bool CanTick() const { return bCanEverTick && bTickEnabled; }
	void SetActorTickEnabled(bool bEnabled) { bTickEnabled = bEnabled; }
	const FVector& GetActorLocation() const;
	void SetActorLocation(const FVector& InLocation);

protected:
	UScene* Scene = nullptr;
	//ULevel* Level = nullptr;

	USceneComponent* RootComponent = nullptr;
	TArray<UActorComponent*> OwnedComponents;

	bool bCanEverTick = true;
	bool bTickEnabled = true;
	bool bActorBegunPlay = false;
	bool bPendingDestroy = false;
};

