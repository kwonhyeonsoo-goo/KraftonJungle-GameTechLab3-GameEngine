#include "SceneManager.h"

void FSceneManager::LoadScene(const FString& SceneName) {
    // TODO:: Deserialization logic
}

void FSceneManager::UnloadScene(UScene* Scene) {
    if (!Scene) return;
    if (Scene == ActiveScene) {
        return;
    }

    Scene->EndPlay();
    Scene->bPendingKill = true;

    LoadedScenes.erase(
        std::remove(LoadedScenes.begin(), LoadedScenes.end(), Scene),
        LoadedScenes.end()
    );
}

void FSceneManager::SetActiveScene(UScene* Scene) {
	ActiveScene = Scene;
}

void FSceneManager::AddNewScene() {
    auto WeakScene = UObjectManager::Get().CreateObject<UScene>();
    UScene* Scene = WeakScene.lock().get();
    LoadedScenes.push_back(Scene);
    SetActiveScene(Scene);
}

void FSceneManager::AddScene(UScene* Scene) {
    if (!Scene) return;
    for (auto& ExistingScene : LoadedScenes) {
        if (ExistingScene == Scene) return;
    }

    LoadedScenes.push_back(Scene);

    if (!ActiveScene) {
        SetActiveScene(Scene);
    }
}

//uint32 FSceneManager::RemoveActiveScene() {  
//   if (LoadedScenes.empty() || !ActiveScene) {  
//       return -1;  
//   }  
//   UScene* CurrentActiveScene = ActiveScene;  
//   LoadedScenes.erase(std::remove_if(LoadedScenes.begin(), LoadedScenes.end(), [CurrentActiveScene](UScene* Scene) {  
//       return Scene->UUID == CurrentActiveScene->UUID;  
//   }), LoadedScenes.end());
//
//   ActiveScene = nullptr;
//}

uint32 FSceneManager::RemoveActiveScene()
{
    if (LoadedScenes.empty() || !ActiveScene)
        return -1;

    UScene* CurrentActiveScene = ActiveScene;

    auto It = std::find_if(LoadedScenes.begin(), LoadedScenes.end(), [CurrentActiveScene](UScene* Scene) {
        return Scene->UUID == CurrentActiveScene->UUID;
        });

    if (It == LoadedScenes.end())
        return -1;

    uint32 RemovedIndex = std::distance(LoadedScenes.begin(), It);

    LoadedScenes.erase(It); // no need for remove_if since you found exactly one

    ActiveScene = nullptr;
    return RemovedIndex;
}

weak_ptr<UScene> FSceneManager::GetActiveScene() {
	if (!ActiveScene) return {};
	return std::dynamic_pointer_cast<UScene>(ActiveScene->shared_from_this());
}