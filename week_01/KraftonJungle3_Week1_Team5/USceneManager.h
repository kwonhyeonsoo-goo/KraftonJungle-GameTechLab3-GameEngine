#pragma once
#include <d3d11.h>
#include <memory>
#include <string>
#include <vector>

#include "Macro.h"
#include "SceneRegistry.h"

class UScene;

class USceneManager
{
public:
	USceneManager();
	~USceneManager();

    NO_COPY(USceneManager);

    bool Initialize(const std::string& startSceneName, ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();
    void Update(float deltaTime) const;
    void Render() const;
    bool ChangeSceneImmediate(const std::string& sceneName);
    bool RequestChangeScene(const std::string& sceneName);
    
    template<typename T>
    bool ChangeSceneImmediate()
    {
        static_assert(std::is_base_of_v<UScene, T>, "T must derive from UScene");

        const auto* Entry = SceneRegistry::Get().FindByType<T>();
        if (Entry == nullptr)
        {
            return false;
        }

        return ChangeSceneImmediate(Entry->Name);
    }

    template<typename T>
    bool RequestChangeScene()
    {
        static_assert(std::is_base_of_v<UScene, T>, "T must derive from UScene");

        const auto* Entry = SceneRegistry::Get().FindByType<T>();
        if (Entry == nullptr)
        {
            return false;
        }

        return RequestChangeScene(Entry->Name);
    }

    UScene* GetCurrentScene() const;
    const std::string& GetCurrentSceneName() const;

    bool HasCurrentScene() const
    {
        return CurrentScene != nullptr;
    }

    bool IsCurrentScene(const std::string& sceneName) const;

    std::vector<std::string> GetRegisteredSceneNames() const;

    bool ProcessPendingSceneChange();

private:
	bool SetScene(std::unique_ptr<UScene> NewScene, const std::string& SceneName);

private:
    std::unique_ptr<UScene> CurrentScene;
    std::string CurrentSceneName;

    bool bChangeRequested = false;
    std::string PendingSceneName;

    ID3D11Device* Device;
    ID3D11DeviceContext* Context;
};

