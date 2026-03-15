#include "ObjectFactory.h"
#include "../../AppContext.h"
#include "../World/UCubeComp.h"
#include "../World/USphereComp.h"
#include "../World/UPlaneComp.h"
#include "../World/UTriangleComp.h"

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
    else if (typeName == "Triangle")
    {
        obj = new UTriangleComp();
    }
    else
    { 
        return nullptr;
    }

    obj->SetUUID(ctx.UUIDs.GenUUID());
    ctx.Objects.Add(obj);
    return obj;
}