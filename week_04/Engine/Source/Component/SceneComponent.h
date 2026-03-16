#pragma once
#include "CoreMinimal.h"
#include "ActorComponent.h"

class ENGINE_API USceneComponent : public UActorComponent
{
public:
	static UClass* StaticClass();

	USceneComponent() : UActorComponent(StaticClass(), "") {}

	USceneComponent(UClass* InClass, const FString& InName, UObject* InOuter = nullptr)
		: UActorComponent(InClass, InName, InOuter)
	{
	}

	const FTransform& GetRelativeTransform() const { return RelativeTransform; }
	void SetRelativeTransform(const FTransform& InTransform);

	const FVector& GetRelativeLocation() const { return RelativeTransform.GetTranslation(); }
	void SetRelativeLocation(const FVector& InLocation);

	USceneComponent* GetAttachParent() const { return AttachParent; }
	const TArray<USceneComponent*>& GetAttachChildren() const { return AttachChildren; }

	void AttachTo(USceneComponent* InParent);
	void DetachFromParent();
	FVector GetWorldLocation() const;
	const FMatrix& GetWorldTransform() const;

private:
	void MarkTransformDirty();
	void UpdateWorldTransform() const;

	FTransform RelativeTransform{};
	USceneComponent* AttachParent = nullptr;
	TArray<USceneComponent*> AttachChildren;

	mutable FMatrix CachedWorldTransform;
	mutable bool bWorldTransformDirty = true;
};

