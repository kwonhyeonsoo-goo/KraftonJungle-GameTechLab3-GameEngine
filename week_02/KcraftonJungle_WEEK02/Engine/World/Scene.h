#pragma once
#include "CoreTypes.h"
#include "UPrimitiveComponent.h"
#include "AppContext.h"

struct AppContext;

class Scene
{
public:
    Scene();

    void Add(UPrimitiveComponent* comp);
    void Remove(uint32 uuid);
    UPrimitiveComponent* Find(uint32 uuid) const;
    const TArray<UObject*>& GetAllObjects() const;

    void SetContext(AppContext* ctx);

private:
    AppContext* Ctx = nullptr;
};