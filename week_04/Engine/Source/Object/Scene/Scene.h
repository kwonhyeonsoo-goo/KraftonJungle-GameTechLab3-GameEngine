#pragma once
#include "Object/Object.h"
#include <d3d11.h>

class AActor;
class CCamera;
class FFrustum;
class UPrimitiveComponent;
struct FRenderCommandQueue;

class ENGINE_API UScene : public UObject
{
public:
	static UClass* StaticClass();

	UScene(UClass* InClass, const FString& InName, UObject* InOuter = nullptr);
	~UScene() override;

	template <typename T>
	T* SpawnActor(const FString& InName)
	{
		static_assert(std::is_base_of_v<AActor, T>, "T must derive from AActor");

		UObject* NewObj = T::StaticClass()->CreateInstance(this, InName);
		if (!NewObj)
		{
			return nullptr;
		}

		T* NewActor = static_cast<T*>(NewObj);
		RegisterActor(NewActor);
		NewActor->PostSpawnInitialize();

		return NewActor;
	}

	void RegisterActor(AActor* InActor);
	void DestroyActor(AActor* InActor);
	void CleanupDestroyedActors();

	const TArray<AActor*>& GetActors() const { return Actors; }

	CCamera* GetCamera() const { return Camera; }

	void InitializeDefaultScene(float AspectRatio, ID3D11Device* Device = nullptr);
	void LoadSceneFromFile(const FString& FilePath, ID3D11Device* Device = nullptr);
	void SaveSceneToFile(const FString& FilePath);
	void ClearActors();
	void BeginPlay();
	void Tick(float DeltaTime);
	void CollectRenderCommands(const FFrustum& Frustum, FRenderCommandQueue& OutQueue);

private:
	// 컬링: 가시 PrimitiveComponent 목록 수집 (멀티스레드 분리 가능)
	void CullVisiblePrimitives(const FFrustum& Frustum, TArray<UPrimitiveComponent*>& OutVisible);
	TArray<AActor*> Actors;
	CCamera* Camera = nullptr;
	bool bBegunPlay = false;
};
