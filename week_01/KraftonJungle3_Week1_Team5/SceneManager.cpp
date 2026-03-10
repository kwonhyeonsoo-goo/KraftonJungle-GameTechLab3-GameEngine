#include "SceneManager.h"

#include "UScene.h"

SceneManager::SceneManager() = default;

SceneManager::~SceneManager()
{
	Shutdown();
}

bool SceneManager::Initialize(const std::string& startSceneName, ID3D11Device* device, ID3D11DeviceContext* context)
{
    Device = device;
    Context = context;
	return ChangeSceneImmediate(startSceneName);
}

void SceneManager::Shutdown()
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

void SceneManager::Update(float deltaTime)
{
    if (CurrentScene)
    {
        CurrentScene->Update(deltaTime);
    }
}

void SceneManager::Render(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    if (CurrentScene)
    {
        CurrentScene->Render(device, deviceContext);
    }
}

bool SceneManager::ChangeSceneImmediate(const std::string& sceneName)
{
    std::unique_ptr<UScene> NewScene = SceneRegistry::Get().CreateSceneByName(sceneName);
    if (NewScene == nullptr)
    {
        return false;
    }

    return SetScene(std::move(NewScene), sceneName);
}

bool SceneManager::RequestChangeScene(const std::string& sceneName)
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

UScene* SceneManager::GetCurrentScene() const
{
    return CurrentScene.get();
}

const std::string& SceneManager::GetCurrentSceneName() const
{
    return CurrentSceneName;
}

bool SceneManager::IsCurrentScene(const std::string& sceneName) const
{
    return CurrentSceneName == sceneName;
}

std::vector<std::string> SceneManager::GetRegisteredSceneNames() const
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

bool SceneManager::ProcessPendingSceneChange()
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

bool SceneManager::SetScene(std::unique_ptr<UScene> NewScene, const std::string& SceneName)
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

    return true;
}

IMPLEMENT_SINGLETON(SceneManager)

