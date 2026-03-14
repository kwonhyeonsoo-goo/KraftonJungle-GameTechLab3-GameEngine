#include "World.h"
#include "../../AppContext.h"

World::World()
    : Ctx(nullptr)
{
}

Level* World::GetPersistentLevel()
{
    return &PersistentLevel;
}

const Level* World::GetPersistentLevel() const
{
    return &PersistentLevel;
}

void World::SetContext(AppContext* ctx)
{
    Ctx = ctx;
}

void World::Add(UPrimitiveComponent* comp)
{
    if (Ctx == nullptr || comp == nullptr)
    {
        return;
    }

    //Ctx->Objects.Add(comp);
}

void World::Remove(uint32 uuid)
{
    if (Ctx == nullptr)
    {
        return;
    }

    //Ctx->Objects.Remove(uuid);
}

UPrimitiveComponent* World::Find(uint32 uuid) const
{
    if (Ctx == nullptr)
    {
        return nullptr;
    }

    return static_cast<UPrimitiveComponent*>(Ctx->Objects.Find(uuid));
}

const TArray<UObject*>& World::GetAllObjects() const
{
    return Ctx->Objects.GUObjectArray();
}