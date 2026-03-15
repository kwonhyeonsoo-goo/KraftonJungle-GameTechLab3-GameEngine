#pragma once
#include "./UObject.h"
#include "../Foundation/Core/CoreTypes.h"
#include "./UClass.h"
#include "../World/UCubeComp.h"
#include "../World/UPlaneComp.h"
#include "../World/USphereComp.h"
#include "../World/UTriangleComp.h"

struct AppContext;

class ObjectFactory
{
public:
    UObject* ConstructObject(AppContext& ctx, UClass* uclass);
    UObject* ConstructObject(AppContext& ctx, const FString& typeName);

    template<typename T>
    T* Construct(AppContext& ctx)
    {
        return static_cast<T*>(ConstructObject(ctx, T::StaticClass()));
    }
};