#include "Scene.h"

DEFINE_CLASS(UScene, UObject)
REGISTER_FACTORY(UScene)

UScene::~UScene() {
    EndPlay();
}

void UScene::BeginPlay() {
    for (AActor* Actor : Actors) {
        if (Actor && !Actor->bPendingKill) {
            Actor->BeginPlay();
        }
    }
}

void UScene::Tick(float DeltaTime) {
    for (AActor* Actor : Actors) {
        if (Actor && !Actor->bPendingKill) {
            Actor->Tick(DeltaTime);
        }
    }
}

void UScene::EndPlay() {
    Actors.clear();
    ActiveCamera = nullptr;
}
