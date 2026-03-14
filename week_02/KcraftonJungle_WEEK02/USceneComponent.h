#pragma once
#include "UObject.h"
#include "Transform.h"

class USceneComponent : public UObject
{
public:
    FVector RelativeLocation;
    FVector RelativeRotation;
    FVector RelativeScale3D;

    USceneComponent();

    Transform GetTransform() const;
    FMatrix GetWorldMatrix() const;
    void SetTransform(const Transform& t);

    USceneComponent* Parent = nullptr;
    TArray<USceneComponent*> Children;

    void AttachTo(USceneComponent* parent);
    void Detach();

    static UClass* StaticClass();
    UClass* GetClass() const override;
};