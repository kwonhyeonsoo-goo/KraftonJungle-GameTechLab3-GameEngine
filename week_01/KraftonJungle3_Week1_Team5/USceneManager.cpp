#include "USceneManager.h"

#include "UScene.h"

USceneManager::USceneManager() = default;

USceneManager::~USceneManager()
{
	Shutdown();
}

bool USceneManager::Initialize(const std::string& startSceneName, ID3D11Device* device, ID3D11DeviceContext* context)
{
    Device = device;
    Context = context;
	return ChangeSceneImmediate(startSceneName);
}

void USceneManager::Shutdown()
{
    bChangeRequested = false;
    PendingSceneName.clear();

    if (CurrentScene)
    {
        CurrentScene->Exit();
        CurrentScene->Release();
        CurrentScene.reset();
        CurrentSceneName.clear();
    }
}

void USceneManager::Update(float deltaTime) const
{
    if (CurrentScene)
    {
        CurrentScene->Update(deltaTime);
    }
}

void USceneManager::Render() const
{
    if (CurrentScene)
    {
        if (Device && Context)
        {
            CurrentScene->Render(Device, Context);
        }
    }
}

bool USceneManager::ChangeSceneImmediate(const std::string& sceneName)
{
    std::unique_ptr<UScene> NewScene = SceneRegistry::Get().CreateSceneByName(sceneName);
    if (NewScene == nullptr)
    {
        return false;
    }

    return SetScene(std::move(NewScene), sceneName);
}

bool USceneManager::RequestChangeScene(const std::string& sceneName)
{
    const auto* Entry = SceneRegistry::Get().FindByName(sceneName);
    if (Entry == nullptr)
    {
        return false;
    }

    bChangeRequested = true;
    PendingSceneName = sceneName;
    return true;
}

UScene* USceneManager::GetCurrentScene() const
{
    return CurrentScene.get();
}

const std::string& USceneManager::GetCurrentSceneName() const
{
    return CurrentSceneName;
}

bool USceneManager::IsCurrentScene(const std::string& sceneName) const
{
    return CurrentSceneName == sceneName;
}

std::vector<std::string> USceneManager::GetRegisteredSceneNames() const
{
    std::vector<std::string> Result;

    const auto& Entries = SceneRegistry::Get().GetEntries();
    Result.reserve(Entries.size());

    for (const auto& Entry : Entries)
    {
        Result.push_back(Entry.Name);
    }

    return Result;
}

bool USceneManager::ProcessPendingSceneChange()
{
    if (!bChangeRequested)
    {
        return false;
    }

    const std::string NextSceneName = PendingSceneName;

    bChangeRequested = false;
    PendingSceneName.clear();

    return ChangeSceneImmediate(NextSceneName);
}

bool USceneManager::SetScene(std::unique_ptr<UScene> NewScene, const std::string& SceneName)
{
    if (NewScene == nullptr)
    {
        return false;
    }

    if (CurrentScene)
    {
        CurrentScene->Exit();
        CurrentScene->Release();
        CurrentScene.reset();
    }

    NewScene->Initialize(Device, Context);

    CurrentScene = std::move(NewScene);
    CurrentSceneName = SceneName;

    CurrentScene->Enter();

    return true;
}

