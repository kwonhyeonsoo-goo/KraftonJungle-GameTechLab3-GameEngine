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
    void SetName(const FString& name) { Name = name; }
    const FString GetName() const {return Name; }
    FString GetName() {return Name; }

private:
    FString Name = "default";
    AppContext* Ctx = nullptr;
public:
    Level PersistentLevel;
    TArray<Level*> SubLevels;
};