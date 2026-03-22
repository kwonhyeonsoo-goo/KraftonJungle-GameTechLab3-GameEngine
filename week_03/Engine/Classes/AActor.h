#pragma once
#include "Object/Object.h"
#include "Object/ObjectFactory.h"
#include "World/SceneComponent.h"

class UScene;

class AActor : public UObject{
public:
	DECLARE_CLASS(AActor, UObject)
	AActor() = default;
	~AActor() override;

	virtual void BeginPlay() {}
	virtual void Tick(float DeltaTime) {}
	virtual void EndPlay() {}

	template<typename T>
	T* AddComponent() {
		auto WeakComp = UObjectManager::Get().CreateObject<T>();
		T* component = WeakComp.lock().get();

		component->SetOwningActor(this);

		// First component added becomes the root
		if (!RootComponent)
		{
			RootComponent = component;
			RootComponent->SetWorldLocation(PendingActorLocation);
		}
		else
		{
			// Attach to root by default
			component->SetParent(RootComponent);
		}

		Components.push_back(component);
		return component;
	}

	USceneComponent* AddComponent(USceneComponent* ExistingComp) {
		if (!ExistingComp) return nullptr;

		if (!RootComponent) {
			RootComponent = ExistingComp;
			RootComponent->SetWorldLocation(PendingActorLocation);
		}
		else {
			ExistingComp->SetParent(RootComponent);
		}

		RegisterComponentRecursive(ExistingComp);
		return ExistingComp;
	}

	void RemoveComponent(USceneComponent* Component){
		if (!Component) return;

		auto it = std::find(Components.begin(),
			Components.end(), Component);
		if (it != Components.end())
			Components.erase(it);

		if (RootComponent == Component)
			RootComponent = Components.empty()
			? nullptr
			: Components[0];

		UObjectManager::Get().DestroyObject(Component);
	}

	void SetRootComponent(USceneComponent* Comp) {
		if (!Comp) return;
		RootComponent = Comp;
		Components.clear(); // rebuild from scratch
		RegisterComponentRecursive(Comp);
	}

	USceneComponent* GetRootComponent() const { return RootComponent; }
	const TArray<USceneComponent*>& GetComponents() const { return Components; }
	FVector GetActorLocation() const;
	void SetActorLocation(const FVector& Location);
	FVector GetActorForward() const
	{
		if (RootComponent)
			return RootComponent->GetForwardVector();
		return FVector(0, 0, 1);
	}

	void SetOwnerUUID(uint32 SceneUUID) { OwningSceneUUID = SceneUUID; }
	uint32 GetOwnerUUID() const { return OwningSceneUUID; }
	//UScene* GetOwner();

	bool IsVisible() const { return bVisible; }
	void SetVisible(bool Visible) { bVisible = Visible; }

protected:
	USceneComponent* RootComponent = nullptr;
	uint32 OwningSceneUUID = -1;

	FVector PendingActorLocation = FVector(0, 0, 0);
	bool bVisible = true;

	TArray<USceneComponent*> Components;
	
private:
	void RegisterComponentRecursive(USceneComponent* Comp);
};
