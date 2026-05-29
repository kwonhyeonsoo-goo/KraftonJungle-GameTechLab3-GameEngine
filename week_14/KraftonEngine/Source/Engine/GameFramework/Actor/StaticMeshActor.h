#pragma once

#include "GameFramework/AActor.h"
#include "Object/Ptr/WeakObjectPtr.h"

#include "Source/Engine/GameFramework/Actor/StaticMeshActor.generated.h"
class UStaticMeshComponent;
class UTextRenderComponent;
class USubUVComponent;

UCLASS()
class AStaticMeshActor : public AActor
{
public:
	GENERATED_BODY()
	AStaticMeshActor() {}

	void BeginPlay() override;

	void InitDefaultComponents(const FString& UStaticMeshFileName = "Content/Data/BasicShape/Cylinder.obj");

private:
	TWeakObjectPtr<UStaticMeshComponent> StaticMeshComponent = nullptr;
	//TWeakObjectPtr<UTextRenderComponent> TextRenderComponent = nullptr;
	//USubUVComponent* SubUVComponent = nullptr;
};
