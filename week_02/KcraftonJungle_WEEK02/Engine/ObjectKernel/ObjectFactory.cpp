#include "ObjectFactory.h"
#include "../../AppContext.h"
#include "../World/UCubeComp.h"
#include "../World/USphereComp.h"
#include "../World/UPlaneComp.h"
#include "../World/UTriangleComp.h"
#include <memory>

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
    std::unique_ptr<UObject> obj;

    if (typeName == "Cube")
    {
        obj = std::make_unique<UCubeComp>();
    }
    else if (typeName == "Sphere")
    {
        obj = std::make_unique<USphereComp>();
    }
    else if (typeName == "Plane")
    {
        obj = std::make_unique<UPlaneComp>();
    }
    else if (typeName == "Triangle")
    {
        obj = std::make_unique<UTriangleComp>();
    }
    else
    {
        return nullptr;
    }

    obj->SetUUID(ctx.UUIDs.GenUUID());
    UObject* rawPtr = obj.get();
    ctx.Objects.Add(std::move(obj));
    return rawPtr;
}