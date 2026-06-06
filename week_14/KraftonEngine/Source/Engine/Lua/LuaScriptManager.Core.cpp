#include "LuaScriptManager.h"

#include "Asset/AssetRegistry.h"
#include "Animation/Instance/LuaAnimInstance.h"
#include "Audio/AudioManager.h"
#include "CameraShake/CameraShakeAsset.h"
#include "CameraShake/CameraShakeManager.h"
#include "Component/ActorComponent.h"
#include "Component/Camera/CameraComponent.h"
#include "Component/Gameplay/BallisticBulletManagerComponent.h"
#include "Component/Input/InputComponent.h"
#include "Component/Light/LightComponent.h"
#include "Component/Movement/MovementComponent.h"
#include "Component/Particle/ParticleSystemComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Component/Primitive/StaticMeshComponent.h"
#include "Component/PrimitiveComponent.h"
#include "Component/SceneComponent.h"
#include "Component/Script/LuaBlueprintComponent.h"
#include "Component/Script/LuaScriptComponent.h"
#include "Core/Logging/Log.h"
#include "Core/Logging/Notification.h"
#include "Core/Types/CollisionTypes.h"
#include "GameFramework/Camera/PlayerCameraManager.h"
#include "GameFramework/GameMode/PlayerController.h"
#include "GameFramework/World.h"
#include "Input/InputKeyCodes.h"
#include "Input/InputSystem.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialManager.h"
#include "Math/Vector.h"
#include "Mesh/MeshManager.h"
#include "Mesh/Static/StaticMesh.h"
#include "Object/GarbageCollection.h"
#include "Object/Object.h"
#include "Platform/Paths.h"
#include "Platform/WindowsWindow.h"
#include "Profiling/Time/Timer.h"
#include "Runtime/Engine.h"
#include "SimpleJSON/json.hpp"
#include "Texture/Texture2D.h"
#include "UI/UIManager.h"
#include "UI/UserWidget.h"
#include "Viewport/GameViewportClient.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <random>
#include <sstream>
#include <windows.h>

FString FLuaScriptManager::ResolveScriptPath(const FString& ScriptFile)
{
    std::wstring FullPath = FPaths::Combine(FPaths::ScriptDir(), FPaths::ToWide(ScriptFile));
    return FPaths::ToUtf8(FullPath);
}

bool FLuaScriptManager::OpenOrCreateScript(const FString& ScriptFile)
{
    std::wstring FullPath = FPaths::Combine(FPaths::ScriptDir(), FPaths::ToWide(ScriptFile));
    if (!std::filesystem::exists(FullPath))
    {
        FPaths::CreateDir(FPaths::ScriptDir());

        const std::wstring TemplatePath = FPaths::Combine(FPaths::ScriptDir(), L"template.lua");
        std::error_code    Error;
        if (std::filesystem::exists(TemplatePath))
        {
            std::filesystem::copy_file(TemplatePath, FullPath, std::filesystem::copy_options::none, Error);
            if (Error)
            {
                UE_LOG("Failed to copy Lua script template: %s", Error.message().c_str());
            }
        }

        if (!std::filesystem::exists(FullPath))
        {
            std::ofstream Out(FullPath);
            if (!Out)
            {
                return false;
            }
        }
    }

    HINSTANCE HInst = ShellExecuteW(nullptr, L"open", FullPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

    if ((INT_PTR)HInst <= 32)
    {
        return false;
    }

    return true;
}

bool FLuaScriptManager::ReadScriptFileContent(const FString& ScriptFile, FString& OutContent)
{
    const std::wstring WidePath = FPaths::Combine(FPaths::ScriptDir(), FPaths::ToWide(ScriptFile));
    std::ifstream      File(WidePath.c_str(), std::ios::binary);
    if (!File.is_open())
    {
        return false;
    }
    std::ostringstream SS;
    SS << File.rdbuf();
    OutContent = SS.str();
    return true;
}

sol::state& FLuaScriptManager::GetState()
{
    return *Lua;
}

bool FLuaScriptManager::IsInitialized()
{
    return Lua != nullptr;
}

FInputSystemSnapshot FLuaScriptManager::GetLuaInputSnapshot()
{
    if (GEngine)
    {
        if (UGameViewportClient* GameViewportClient = GEngine->GetGameViewportClient())
        {
            if (GameViewportClient->HasGameInputSnapshot())
            {
                return GameViewportClient->GetGameInputSnapshot();
            }
            return FInputSystemSnapshot {};
        }
    }

    return InputSystem::Get().MakeSnapshot();
}

void FLuaScriptManager::SetOnEscapePressed(sol::protected_function Callback)
{
    OnEscapePressedCallback = std::move(Callback);
}

void FLuaScriptManager::FireOnEscapePressed()
{
    if (!OnEscapePressedCallback.valid())
    {
        return;
    }
    FScopedGarbageCollectionBlocker GCBlocker;
    sol::protected_function_result  Result = OnEscapePressedCallback();
    if (!Result.valid())
    {
        sol::error Err = Result;
        UE_LOG("[Lua] OnEscapePressed callback error: %s", Err.what());
    }
}

void FLuaScriptManager::FireWorldReset()
{
    if (!Lua) return;

    sol::table Loaded = (*Lua)["package"]["loaded"];
    if (!Loaded.valid()) return;

    if (sol::object Coro = Loaded["CoroutineManager"]; Coro.valid() && Coro.get_type() == sol::type::table)
    {
        Coro.as<sol::table>()["coroutines"] = Lua->create_table();
    }

    if (sol::object Reg = Loaded["ObjRegistry"]; Reg.valid() && Reg.get_type() == sol::type::table)
    {
        sol::table T    = Reg.as<sol::table>();
        T["car"]        = sol::nil;
        T["carCamera"]  = sol::nil;
        T["carGas"]     = sol::nil;
        T["manObj"]     = sol::nil;
        T["manCamera"]  = sol::nil;
        T["gasNozzle"]  = sol::nil;
        T["carWasher"]  = sol::nil;
        T["dirtyCar"]   = sol::nil;
        T["policeCars"] = Lua->create_table();
    }
}

void FLuaScriptManager::RegisterComponent(ULuaScriptComponent* Component)
{
    if (!IsAliveObject(Component)) return;

    std::lock_guard<std::mutex> Lock(ComponentMutex);
    for (const TWeakObjectPtr<ULuaScriptComponent>& Existing : RegisteredComponents)
    {
        if (Existing.Get() == Component)
        {
            return;
        }
    }
    RegisteredComponents.push_back(Component);
}

void FLuaScriptManager::UnregisterComponent(ULuaScriptComponent* Component)
{
    if (!Component) return;

    std::lock_guard<std::mutex> Lock(ComponentMutex);
    RegisteredComponents.erase(
        std::remove_if(
            RegisteredComponents.begin(),
            RegisteredComponents.end(),
            [Component](const TWeakObjectPtr<ULuaScriptComponent>& Existing)
            {
                return Existing.Get() == Component || !Existing.IsValid();
            }
        ),
        RegisteredComponents.end()
    );
}

void FLuaScriptManager::RegisterAnimInstance(ULuaAnimInstance* Instance)
{
    if (!IsAliveObject(Instance)) return;
    std::lock_guard<std::mutex> Lock(ComponentMutex);
    for (const TWeakObjectPtr<ULuaAnimInstance>& Existing : RegisteredAnimInstances)
    {
        if (Existing.Get() == Instance)
        {
            return;
        }
    }
    RegisteredAnimInstances.push_back(Instance);
}

void FLuaScriptManager::UnregisterAnimInstance(ULuaAnimInstance* Instance)
{
    if (!Instance) return;
    std::lock_guard<std::mutex> Lock(ComponentMutex);
    RegisteredAnimInstances.erase(
        std::remove_if(
            RegisteredAnimInstances.begin(),
            RegisteredAnimInstances.end(),
            [Instance](const TWeakObjectPtr<ULuaAnimInstance>& Existing)
            {
                return Existing.Get() == Instance || !Existing.IsValid();
            }
        ),
        RegisteredAnimInstances.end()
    );
}
