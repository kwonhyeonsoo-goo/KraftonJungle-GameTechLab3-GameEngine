#pragma once
#include "CoreMinimal.h"
#include "ActorComponent.h"

class ENGINE_API USceneComponent : public UActorComponent
{
public:
	static UClass* StaticClass();

	USceneComponent() : UActorComponent(nullptr, "", nullptr) {}

	USceneComponent(UClass* InClass, const FString& InName, UObject* InOuter = nullptr)
		: UActorComponent(InClass, InName, InOuter)
	{
	}

	const FTransform& GetRelativeTransform() const { return RelativeTransform; }
	void SetRelativeTransform(const FTransform& InTransform) { RelativeTransform = InTransform; }

	const FVector& GetRelativeLocation() const { return RelativeTransform.Location; }
	void SetRelativeLocation(const FVector& InLocation) { RelativeTransform.Location = InLocation; }

	USceneComponent* GetAttachParent() const { return AttachParent; }
	const TArray<USceneComponent*>& GetAttachChildren() const { return AttachChildren; }

	void AttachTo(USceneComponent* InParent);
	void DetachFromParent();
	FVector GetWorldLocation() const;

private:
	FTransform RelativeTransform {};
	USceneComponent* AttachParent = nullptr;
	TArray<USceneComponent*> AttachChildren;
};

