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

void FSceneManager::RemoveActiveScene() {  
   if (LoadedScenes.empty() || !ActiveScene) {  
       return;  
   }  
   UScene* CurrentActiveScene = ActiveScene;  
   LoadedScenes.erase(std::remove_if(LoadedScenes.begin(), LoadedScenes.end(), [CurrentActiveScene](UScene* Scene) {  
       return Scene->UUID == CurrentActiveScene->UUID;  
   }), LoadedScenes.end());

   ActiveScene = nullptr;
}

weak_ptr<UScene> FSceneManager::GetActiveScene() {
	if (!ActiveScene) return {};
	return std::dynamic_pointer_cast<UScene>(ActiveScene->shared_from_this());
}