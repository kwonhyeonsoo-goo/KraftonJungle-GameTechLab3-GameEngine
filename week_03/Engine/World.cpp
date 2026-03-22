#include "World.h"

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

