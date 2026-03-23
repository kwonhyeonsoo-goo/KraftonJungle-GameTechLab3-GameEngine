#pragma once
#include "LightComponent.h"
#include "World/Primitives/Primitives.h"
#include "Core/Common.h"
#include "Render/Scene/BatchedLine.h"

class USpotlightComponent : public ULightComponent {
private:
	// Cone stuffs
	float ConeHeight = common::constants::Actors::ASpotlight::DefaultConeHeight;
	float Radius = common::constants::Actors::ASpotlight::DefaultRadius;
	float Yaw = 0.0f;
	float Pitch = 0.0f;
	int NumCircleVertex = common::constants::Actors::ASpotlight::DefaultCircleVertex;

	FVector UnitNormal;
	FVector UnitOrthogonal;
	FVector CircleCenter;
	FVector4 ConeColor = FVector4(1.0f, 1.0f, 0.0f, 1.0f);	// Yellow by default

	// Checks if spotlight cone has been transformed
	bool bIsConeDirty = false;

	TArray<FVector> CircleVertices;

public:
	DECLARE_CLASS(USpotlightComponent, USceneComponent)

	USpotlightComponent();
	~USpotlightComponent() = default;

	void BeginPlay() override {}
	void TickComponent(float DeltaTime) override;
	void EndPlay() override {}

	void UpdateCone();
	void SetConeHeight(float NewConeHeight) { ConeHeight = NewConeHeight; bIsConeDirty = true; }
	void SetConeRadius(float NewRadius) { Radius = NewRadius; bIsConeDirty = true; }
	void SetNumVertex(size_t Num) { NumCircleVertex = Num; bIsConeDirty = true; }
	void SetConeColor(FVector4 NewConeColor) { ConeColor = NewConeColor; }
	void SetYaw(float NewYaw) { Yaw = NewYaw; bIsConeDirty = true; }
	void SetPitch(float NewPitch) { Pitch = NewPitch; bIsConeDirty = true; }
	void SetRadius(float NewRadius) { if (NewRadius >= 0) Radius = NewRadius; bIsConeDirty = true; }

	float GetYaw() const { return Yaw; }
	float GetPitch() const { return Pitch; }
	float GetConeHeight() const { return ConeHeight; }
	float GetRadius() const { return Radius; }
	int GetNumVertices() const { return NumCircleVertex; }

	//void Transformed() override { bIsConeDirty = true; }

	// Rendering
	void AddConeLinesToBatch(FBatchedLine* BatchLine);
};
