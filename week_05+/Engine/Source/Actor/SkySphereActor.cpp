#include "SkySphereActor.h"
#include "Object/ObjectFactory.h"
#include "World/Level.h"
#include "Component/CameraComponent.h"
#include "Renderer/MaterialManager.h"
#include "Component/StaticMeshComponent.h"
#include "Camera/Camera.h"
#include "Object/Class.h"


IMPLEMENT_RTTI(ASkySphereActor, AActor)

void ASkySphereActor::PostSpawnInitialize()
{
	StaticMeshComponent = FObjectFactory::ConstructObject<UStaticMeshComponent>(this, "SkyMesh");
	SetRootComponent(StaticMeshComponent);
	AddOwnedComponent(StaticMeshComponent);

	// 스카이 스피어 크기 뻥튀기
	FTransform T = StaticMeshComponent->GetRelativeTransform();
	T.SetScale3D({ 2000.0f, 2000.0f, 2000.0f });
	StaticMeshComponent->SetRelativeTransform(T);

	AActor::PostSpawnInitialize();
}

void ASkySphereActor::Tick(float DeltaTime)
{
	AActor::Tick(DeltaTime);

	if (ULevel* Level = GetLevel())
	{
		if (FCamera* Camera = Level->GetCamera())
		{
			if (USceneComponent* Root = GetRootComponent())
			{
				FTransform T = Root->GetRelativeTransform();
				T.SetTranslation(Camera->GetPosition());
				Root->SetRelativeTransform(T);
			}
		}
	}
}

void ASkySphereActor::LoadSkyMesh(ID3D11Device* Device)
{
	if (!StaticMeshComponent) return;

	//방금 에셋매니저에 등록한 SkySphere 에셋을 로드합니다.
	StaticMeshComponent->LoadStaticMesh(Device, "Engine/BasicShapes/SkySphere");

	//하늘 머티리얼(M_Sky)을 덮어씌웁니다.
	std::shared_ptr<FMaterial> SkyMat = FMaterialManager::Get().FindByName("M_Sky");
	if (SkyMat)
	{
		StaticMeshComponent->SetMaterial(0, SkyMat.get());
	}
}
