#pragma once
#include "CoreMinimal.h"
#include "Object/Object.h"
#include "Scene/SceneTypes.h"

// Forward declarations — include 최소화
class UScene;
class AActor;
class UCameraComponent;
class CCamera;
class FFrustum;
struct FRenderCommandQueue;
struct ID3D11Device;

class ENGINE_API UWorld : public UObject
{
public:
	DECLARE_RTTI(UWorld, UObject)
	~UWorld();

	template <typename T>
	T* SpawnActor(const FString& InName);

	void DestroyActor(AActor* InActor);
	const TArray<AActor*>& GetActors() const;

	// 카메라
	void SetActiveCameraComponent(UCameraComponent* InCamera);
	UCameraComponent* GetActiveCameraComponent() const;
	CCamera* GetCamera() const;

	// 라이프사이클
	void InitializeWorld(float AspectRatio, ID3D11Device* Device = nullptr);
	void BeginPlay();
	void Tick(float InDeltaTime);
	void CleanupWorld();


	// 접근자
	UScene* GetScene() const { return Scene; }
	ESceneType GetWorldType() const { return WorldType; }
	void SetWorldType(ESceneType InType) { WorldType = InType; }
	float GetWorldTime() const { return WorldTime; }
	float GetDeltaTime() const { return DeltaSeconds; }

private:
	UScene* Scene = nullptr;
	float WorldTime = 0.f;
	float DeltaSeconds = 0.f;
	ESceneType WorldType = ESceneType::Game;
};
#include "Scene/Scene.h"

template <typename T>
T* UWorld::SpawnActor(const FString& InName)
{
	static_assert(std::is_base_of_v<AActor, T>, "T must derive from AActor");
	if (!Scene) return nullptr;
	return Scene->SpawnActor<T>(InName);
}