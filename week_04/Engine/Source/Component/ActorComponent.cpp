#include "ActorComponent.h"

#include "Object/Class.h"

namespace
{
    UObject* CreateUActorComponentInstance(UObject* InOuter, const FString& InName)
    {
        return new UActorComponent(UActorComponent::StaticClass(), InName, InOuter);
    }
}

UClass* UActorComponent::StaticClass()
{
    static UClass ClassInfo("UActorComponent", UObject::StaticClass(), &CreateUActorComponentInstance);
    return &ClassInfo;
}