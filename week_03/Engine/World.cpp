#include "World.h"
#include <SimpleJSON/json.hpp>

DEFINE_CLASS(UWorld, UObject)
REGISTER_FACTORY(UWorld)

UWorld::~UWorld(){
	EndPlay();
}

void UWorld::Init() {
	// Create a new scene if scene list in SceneManager is empty. It should be implicitly set active.
	if (SceneManager.IsEmpty()) {
		auto WeakScene = UObjectManager::Get().CreateObject<UScene>();
		SceneManager.AddScene(WeakScene.lock().get());
	}
}

void UWorld::BeginPlay(){
	if (auto Scene = SceneManager.GetActiveScene().lock()) Scene->BeginPlay();
}

void UWorld::Tick(float DeltaTime) {
	if (auto Scene = SceneManager.GetActiveScene().lock()) Scene->Tick(DeltaTime);
}

void UWorld::EndPlay() {
	if (auto Scene = SceneManager.GetActiveScene().lock()) Scene->EndPlay();
}

void UWorld::Serialize(json::JSON& j) const {
	SerializeHeader(j);
	json::JSON SceneArray = json::Array();
	for (UScene* Scene : SceneManager.GetScenes()) {
		Scene->Serialize(SceneArray);
	}
}

void UWorld::Deserialize(const json::JSON& j) {

}