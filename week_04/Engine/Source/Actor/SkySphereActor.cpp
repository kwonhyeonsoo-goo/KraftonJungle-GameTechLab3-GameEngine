#include "SkySphereActor.h"
#include "Component/SkyComponent.h"
#include "Object/ObjectFactory.h"
#include "World/Level.h"
#include "Renderer/MaterialManager.h"
#include "Object/Class.h"

IMPLEMENT_RTTI(ASkySphereActor, AActor)

void ASkySphereActor::PostSpawnInitialize()
{
	SkyComponent = FObjectFactory::ConstructObject<USkyComponent>(this);
	AddOwnedComponent(SkyComponent);

	std::shared_ptr<FMaterial> SkyMat = FMaterialManager::Get().FindByName("M_Sky");
	if (SkyMat)
	{
		SkyComponent->SetMaterial(SkyMat.get());
	}

	if (USceneComponent* Root = GetRootComponent())
	{
		FTransform T = Root->GetRelativeTransform();
		T.SetScale3D({ 2000.0f, 2000.0f, 2000.0f });
		Root->SetRelativeTransform(T);
	}

	AActor::PostSpawnInitialize();
}

void ASkySphereActor::Tick(float DeltaTime)
{
	AActor::Tick(DeltaTime);

	// 카메라 위치를 따라가는 로직은 World가 카메라를 소유하지 않으므로
	// ViewportClient 레이어에서 처리하거나, 렌더 전 위치 힌트를 받는 구조로 개선 필요
	// 현재는 SetCameraPosition()을 외부에서 호출해 위치를 갱신하는 방식으로 대체

	if (bHasCameraPosition)
	{
		if (USceneComponent* Root = GetRootComponent())
		{
			FTransform T = Root->GetRelativeTransform();
			T.SetTranslation(CameraPosition);
			T.SetScale3D({ 2000.0f, 2000.0f, 2000.0f });
			Root->SetRelativeTransform(T);
		}
	}
}

void ASkySphereActor::SetCameraPosition(const FVector& InPosition)
{
	CameraPosition = InPosition;
	bHasCameraPosition = true;
}