#include "SpotLightComponent.h"

DEFINE_CLASS(USpotlightComponent, USceneComponent)
REGISTER_FACTORY(USpotlightComponent)

USpotlightComponent::USpotlightComponent() {
	// Create PlaneComponent as a fixed root
	//AddComponent<UPlaneComponent>();
	UpdateCone();
}

void USpotlightComponent::UpdateCone() {
	CircleVertices.clear();
	CircleVertices.reserve(NumCircleVertex);

	// Base vectors
	FVector BaseNormal = GetUpVector().Normalized();
	FVector BaseRight = GetRightVector().Normalized();
	FVector BaseForward = GetForwardVector().Normalized();

	// Apply pitch around the local right axis, then yaw around the local up axis
	FMatrix PitchMat = FMatrix::MakeRotationAxis(BaseRight, Pitch);
	FMatrix YawMat = FMatrix::MakeRotationAxis(BaseForward, Yaw);
	FMatrix RotMat = PitchMat * YawMat;

	UnitNormal = (RotMat.TransformVector(BaseNormal)).Normalized();
	CircleCenter = GetWorldLocation() + UnitNormal * ConeHeight;

	// Recompute U and V perpendicular to the rotated axis
	FVector U = (RotMat.TransformVector(BaseForward)).Normalized();
	FVector V = UnitNormal.Cross(U).Normalized();

	UnitOrthogonal = U;

	float Angle = 2.0f * M_PI / NumCircleVertex;
	for (size_t i = 0; i < NumCircleVertex; i++) {
		FVector Point = CircleCenter + (U * cosf(Angle * i) + V * sinf(Angle * i)) * Radius;
		CircleVertices.push_back(Point);
	}
}

void USpotlightComponent::AddConeLinesToBatch(FBatchedLine* BatchLine) {
	if (!BatchLine || CircleVertices.empty()) {
		return;
	}

	FVector ConeApex = GetWorldLocation();
	for (size_t i = 0; i < NumCircleVertex; i++) {
		// Point to Apex 
		BatchLine->AddLineRaw(CircleVertices[i], ConeApex, ConeColor);

		// Point to next Point
		BatchLine->AddLineRaw(CircleVertices[i], CircleVertices[(i + 1) % NumCircleVertex], ConeColor);
	}
}

void USpotlightComponent::TickComponent(float DeltaTIme) {
	if (bIsConeDirty) {
		UpdateCone();
		bIsConeDirty = false;
	}
}
