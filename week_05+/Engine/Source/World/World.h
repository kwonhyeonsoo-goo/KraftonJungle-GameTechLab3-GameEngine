#pragma once
#include "CoreMinimal.h"
#include "Object/Object.h"
#include "World/WorldType.h"

// Forward declarations ? include 최소화
class ULevel;
class AActor;
class APawn;
class AController;
class UCameraComponent;
class FCamera;
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

	ULevel* GetLevel() const { return Level; }

	TArray<AActor*> GetActors() const;

	// 카메라
	void SetActiveCameraComponent(UCameraComponent* InCamera);
	UCameraComponent* GetActiveCameraComponent() const;
	FCamera* GetCamera() const;

	// 라이프사이클
	void InitializeWorld();
	void BeginPlay();
	void Tick(float InDeltaTime);
	void CleanupWorld();
	
	EWorldType GetWorldType() const { return WorldType; }
	void SetWorldType(EWorldType InType) { WorldType = InType; }
	float GetWorldTime() const { return WorldTime; }
	float GetDeltaTime() const { return DeltaSeconds; }

	AController* GetDefaultController() const { return DefaultController; }
	APawn* GetDefaultPawn() const { return DefaultPawn; }

	static UWorld* DuplicateWorldForPIE(UWorld* SourceWorld);

private:
	ULevel* Level = nullptr;
	EWorldType WorldType = EWorldType::Game;

	bool bBegunPlay = false;
	float WorldTime = 0.f;
	float DeltaSeconds = 0.f;
	UCameraComponent* LevelCameraComponent = nullptr;
	TObjectPtr<UCameraComponent> ActiveCameraComponent;

	AController* DefaultController = nullptr;
	APawn* DefaultPawn = nullptr;
};
#include "World/Level.h"

template <typename T>
T* UWorld::SpawnActor(const FString& InName)
{
	static_assert(std::is_base_of_v<AActor, T>, "T must derive from AActor");
	if (!Level) return nullptr;
	return Level->SpawnActor<T>(InName);
}

extern ENGINE_API UWorld* GWorld;
