#include "Scene.h"
#include "ObjectStore.h"

Scene::Scene()
    : Ctx(nullptr)
{
}

void Scene::SetContext(AppContext* ctx)
{
    Ctx = ctx;
}

void Scene::Add(UPrimitiveComponent* comp)
{
    if (Ctx == nullptr || comp == nullptr)
    {
        return;
    }

    Ctx->Objects.Add(comp);
}

void Scene::Remove(uint32 uuid)
{
    if (Ctx == nullptr)
    {
        return;
    }

    Ctx->Objects.Remove(uuid);
}

UPrimitiveComponent* Scene::Find(uint32 uuid) const
{
    if (Ctx == nullptr)
    {
        return nullptr;
    }

    return static_cast<UPrimitiveComponent*>(Ctx->Objects.Find(uuid));
}

const TArray<UObject*>& Scene::GetAllObjects() const
{
    return Ctx->Objects.GUObjectArray();
}