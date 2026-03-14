#include "UScene.h"

UScene::UScene()
    : Ctx(nullptr)
{
}

void UScene::SetContext(AppContext* ctx)
{
    Ctx = ctx;
}

void UScene::Add(UPrimitiveComponent* comp)
{
    if (Ctx == nullptr || comp == nullptr)
    {
        return;
    }

    //Ctx->Objects.Add(comp);
}

void UScene::Remove(uint32 uuid)
{
    if (Ctx == nullptr)
    {
        return;
    }

    //Ctx->Objects.Remove(uuid);
}

UPrimitiveComponent* UScene::Find(uint32 uuid) const
{
    if (Ctx == nullptr)
    {
        return nullptr;
    }

    return static_cast<UPrimitiveComponent*>(Ctx->Objects.Find(uuid));
}

const TArray<UObject*>& UScene::GetAllObjects() const
{
    return Ctx->Objects.GUObjectArray();
}