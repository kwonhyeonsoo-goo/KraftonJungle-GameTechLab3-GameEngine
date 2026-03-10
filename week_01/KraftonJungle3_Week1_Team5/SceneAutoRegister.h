#pragma once
#include <cassert>

#include "SceneRegistry.h"

template<typename T>
struct SceneAutoRegister
{
    SceneAutoRegister(const char* Name)
    {
        const bool bRegistered = SceneRegistry::Get().Register<T>(Name);
        assert(bRegistered && "Scene registration failed. Duplicate name or type.");
    }
};

#define REGISTER_SCENE(SceneType) \
    namespace { \
        SceneAutoRegister<SceneType> AutoRegister_##SceneType(#SceneType); \
    }