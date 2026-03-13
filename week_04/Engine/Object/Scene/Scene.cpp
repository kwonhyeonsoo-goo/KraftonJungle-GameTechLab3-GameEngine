#include "Scene.h"

#include "Object/Class.h"

namespace
{
    UObject* CreateUSceneInstance(UObject* InOuter, const std::string& InName)
    {
        return new UScene(UScene::StaticClass(), InName, InOuter);
    }
}

UClass* UScene::StaticClass()
{
    static UClass ClassInfo("UWorld", UObject::StaticClass(), &CreateUSceneInstance);
    return &ClassInfo;
}
