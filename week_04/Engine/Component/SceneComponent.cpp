#include "SceneComponent.h"
#include "Object/Class.h"

namespace
{
    UObject* CreateUSceneComponentInstance(UObject* InOuter, const FString& InName)
    {
        return new USceneComponent(USceneComponent::StaticClass(), InName, InOuter);
    }
}

UClass* USceneComponent::StaticClass()
{
    static UClass ClassInfo("USceneComponent", UActorComponent::StaticClass(), &CreateUSceneComponentInstance);
    return &ClassInfo;
}

void USceneComponent::AttachTo(USceneComponent* InParent)
{
	if (AttachParent == InParent)
	{
		return;
	}

	DetachFromParent();

	AttachParent = InParent;
	if (AttachParent)
	{
		AttachParent->AttachChildren.push_back(this);
	}
}

void USceneComponent::DetachFromParent()
{
	if (AttachParent == nullptr)
	{
		return;
	}

	auto& Siblings = AttachParent->AttachChildren;
	std::erase(Siblings, this);
	AttachParent = nullptr;
}

FVector USceneComponent::GetWorldLocation() const
{
	if (AttachParent == nullptr)
	{
		return RelativeTransform.Location;
	}

	// TODO : XMMatrix 사용해서 제대로 구현해야함.
	const FVector ParentWorld = AttachParent->GetWorldLocation();
	return FVector{
		ParentWorld.X + RelativeTransform.Location.X,
		ParentWorld.Y + RelativeTransform.Location.Y,
		ParentWorld.Z + RelativeTransform.Location.Z
	};
}
