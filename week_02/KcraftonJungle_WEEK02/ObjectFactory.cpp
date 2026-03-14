#include "ObjectFactory.h"

UObject* ObjectFactory::ConstructObject(AppContext& ctx, UClass* uclass)
{
    if (uclass == nullptr)
    {
        return nullptr;
    }

    return ConstructObject(ctx, uclass->ClassName);
}

UObject* ObjectFactory::ConstructObject(AppContext& ctx, const FString& typeName)
{
    UObject* obj = nullptr;

    if (typeName == "Cube")
    {
        obj = new UCubeComp();
    }
    else if (typeName == "Sphere")
    {
        obj = new USphereComp();
    }
    else if (typeName == "Plane")
    {
        obj = new UPlaneComp();
    }
    else
    {
        return nullptr;
    }

    obj->SetUUID(ctx.UUIDs.GenUUID());
    ctx.Objects.Add(obj);
    return obj;
}