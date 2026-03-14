#pragma once
#include "../Foundation/Core/CoreTypes.h"
#include "UPrimitiveComponent.h"
#include "../../AppContext.h"
#include "../ObjectKernel/UObject.h"

struct AppContext;

class UScene
{
public:
    UScene();

    void Add(UPrimitiveComponent* comp);
    void Remove(uint32 uuid);
    UPrimitiveComponent* Find(uint32 uuid) const;
    const TArray<UObject*>& GetAllObjects() const;

    void SetContext(AppContext* ctx);
    void SetName(const FString& name) { Name = name; }
    const FString GetName() const {return Name; }

private:
    FString Name;
    AppContext* Ctx = nullptr;
};