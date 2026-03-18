#include "USceneComponent.h"

USceneComponent::USceneComponent()
    : RelativeLocation(FVector::Zero)
    , RelativeRotation(FQuat::Identity())
    , RelativeScale3D(FVector::One)
    , Parent(nullptr)
{
}

Transform USceneComponent::GetTransform() const
{
    return Transform(RelativeLocation, RelativeRotation, RelativeScale3D);
}

FMatrix USceneComponent::GetWorldMatrix() const
{
    const FMatrix local = GetTransform().ToMatrix();

    if (Parent == nullptr)
    {
        return local;
    }

    return local * Parent->GetWorldMatrix();
}

void USceneComponent::SetTransform(const Transform& t)
{
    RelativeLocation = t.Location;
    RelativeRotation = t.Rotation.Normalized();
    RelativeScale3D = t.Scale.ClampedMin(1e-6f);
}

void USceneComponent::AttachTo(USceneComponent* parent)
{
    if (Parent == parent)
    {
        return;
    }

    Detach();
    Parent = parent;

    if (Parent == nullptr)
    {
        return;
    }

    for (USceneComponent* child : Parent->Children)
    {
        if (child == this)
        {
            return;
        }
    }

    Parent->Children.push_back(this);
}

void USceneComponent::Detach()
{
    if (Parent == nullptr)
    {
        return;
    }

    auto& siblings = Parent->Children;
    for (auto it = siblings.begin(); it != siblings.end(); ++it)
    {
        if (*it == this)
        {
            siblings.erase(it);
            break;
        }
    }

    Parent = nullptr;
}

UClass* USceneComponent::StaticClass()
{
    static UClass ClassInfo("SceneComponent", UObject::StaticClass());
    return &ClassInfo;
}

UClass* USceneComponent::GetClass() const
{
    return USceneComponent::StaticClass();
}
