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

private:
    AppContext* Ctx = nullptr;
public:
	std::string Name;

};