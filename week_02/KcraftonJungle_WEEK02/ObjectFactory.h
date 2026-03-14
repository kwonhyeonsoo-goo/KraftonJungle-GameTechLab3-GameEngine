#pragma once
#include "./UObject.h"
#include "./CoreTypes.h"
#include "./UClass.h"
#include "UCubeComp.h"
#include "USphereComp.h"
#include "UPlaneComp.h"

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