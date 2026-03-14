#pragma once
#include "../Foundation/Core/CoreTypes.h"
#include "UPrimitiveComponent.h"
#include "../ObjectKernel/UObject.h"
#include "Level.h"

struct AppContext;

class World
{
public:
    World();
    Level* GetPersistentLevel();
    const Level* GetPersistentLevel() const;

    void Add(UPrimitiveComponent* comp);
    void Remove(uint32 uuid);
    UPrimitiveComponent* Find(uint32 uuid) const;
    const TArray<UObject*>& GetAllObjects() const;

    void SetContext(AppContext* ctx);

private:
    AppContext* Ctx = nullptr;
public:
	std::string Name;
    Level PersistentLevel;
    TArray<Level*> SubLevels;
};