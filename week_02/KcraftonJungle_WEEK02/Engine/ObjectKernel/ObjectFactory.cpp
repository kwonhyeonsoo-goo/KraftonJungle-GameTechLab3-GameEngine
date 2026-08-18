#include "ObjectFactory.h"
#include "../../AppContext.h"
#include <memory>

UObject* ObjectFactory::ConstructObject(AppContext& ctx, const FString& typeName)
{
    UClass* uclass = ctx.Classes.Find(typeName);
    return ConstructObject(ctx, uclass);
}

UObject* ObjectFactory::ConstructObject(AppContext& ctx, UClass* uclass)
{
    if (uclass == nullptr || !uclass->CanInstantiate())
        return nullptr;

    auto obj = uclass->Constructor();
    obj->SetUUID(ctx.UUIDs.GenUUID());
    UObject* raw = obj.get();
    ctx.Objects.Add(std::move(obj));
    return raw;
}