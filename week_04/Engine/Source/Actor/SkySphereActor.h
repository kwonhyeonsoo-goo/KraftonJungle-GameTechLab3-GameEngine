#pragma once

#include "Actor.h"

class USkyComponent;

class ENGINE_API ASkySphereActor : public AActor
{
public:
	DECLARE_RTTI(ASkySphereActor, AActor)

	void PostSpawnInitialize() override;
	void Tick(float DeltaTime) override;

	USkyComponent* GetSkyComponent() const { return SkyComponent; }

	// Core::Render() 루프에서 뷰포트별 카메라 위치를 주입
	// 다중 뷰포트에서는 첫 번째(주) 뷰포트 카메라 기준으로 설정
	void SetCameraPosition(const FVector& InPosition);

private:
	USkyComponent* SkyComponent = nullptr;
	FVector        CameraPosition = FVector::ZeroVector;
	bool           bHasCameraPosition = false;
};