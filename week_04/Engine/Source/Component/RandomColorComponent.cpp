#include "RandomColorComponent.h"
#include "PrimitiveComponent.h"
#include "Object/Actor/Actor.h"
#include "Renderer/Material.h"
#include <random>

namespace
{
	UObject* CreateURandomColorComponentInstance(UObject* InOuter, const FString& InName)
	{
		return new URandomColorComponent();
	}

	FVector4 GenerateRandomColor()
	{
		static std::mt19937 Rng(std::random_device{}());
		static std::uniform_real_distribution<float> Dist(0.0f, 1.0f);
		return { Dist(Rng), Dist(Rng), Dist(Rng), 1.0f };
	}
}

UClass* URandomColorComponent::StaticClass()
{
	static UClass ClassInfo("URandomColorComponent", UActorComponent::StaticClass(), &CreateURandomColorComponentInstance);
	return &ClassInfo;
}

URandomColorComponent::URandomColorComponent()
	: UActorComponent(StaticClass(), "")
{
	bCanEverTick = true;
}

void URandomColorComponent::BeginPlay()
{
	UActorComponent::BeginPlay();

	if (Owner)
	{
		CachedPrimitive = Owner->GetComponentByClass<UPrimitiveComponent>();
	}

	// 시작 시 즉시 한 번 적용
	ApplyRandomColor();
}

void URandomColorComponent::Tick(float DeltaTime)
{
	ElapsedTime += DeltaTime;
	if (ElapsedTime >= UpdateInterval)
	{
		ElapsedTime -= UpdateInterval;
		ApplyRandomColor();
	}
}

void URandomColorComponent::ApplyRandomColor()
{
	if (!CachedPrimitive)
	{
		return;
	}

	FMaterial* Mat = CachedPrimitive->GetMaterial();
	if (!Mat)
	{
		return;
	}

	FVector4 Color = GenerateRandomColor();
	Mat->SetVectorParameter("BaseColor", Color);
}
