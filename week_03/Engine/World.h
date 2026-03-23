#pragma once
#include "Object/Object.h"
#include "Scene/Scene.h"
#include "Classes/AActor.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Core/InputSystem.h"
#include "Scene/SceneManager.h"

class UWorld : public UObject {
public:
    DECLARE_CLASS(UWorld, UObject)
    UWorld() = default;
    ~UWorld() override;

    void Init();
    void BeginPlay();
    void Tick(float  DeltaTime);
    void EndPlay();

    FSceneManager& GetSceneManager() { return SceneManager; }
    weak_ptr<UScene> GetActiveScene() { return SceneManager.GetActiveScene(); }

    template <typename T>
    AActor* SpawnPrimitiveActor(const FVector& Location) {
        AActor* Actor = SceneManager.SpawnPrimitiveActor<T>(Location);
        Actor->BeginPlay();
        return Actor;
    }

private:
    FSceneManager SceneManager;     // Has no independent lifetime = value member
    UScene* CurrentScene = nullptr;
};