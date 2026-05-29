#pragma once

#include "GameFramework/AActor.h"
#include "Object/Ptr/WeakObjectPtr.h"

#include "Source/Engine/GameFramework/Actor/SkeletalMeshActor.generated.h"
class USkeletalMeshComponent;

UCLASS()
class ASkeletalMeshActor : public AActor
{
public:
	GENERATED_BODY()
	ASkeletalMeshActor() = default;

	void BeginPlay() override;

	void InitDefaultComponents(const FString& SkeletalMeshFileName = "Content/Data/Samba Dancing (10).fbx");

private:
	TWeakObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent = nullptr;
};