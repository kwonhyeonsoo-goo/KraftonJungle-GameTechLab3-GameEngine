#include "ASpotlight.h"

DEFINE_CLASS(ASpotlight, AActor)
REGISTER_FACTORY(ASpotlight)

ASpotlight::ASpotlight() {
	// Create PlaneComponent as a fixed root
	AddComponent<UPlaneComponent>();
	UpdateCone();
}

void ASpotlight::UpdateCone() {
	CircleVertices.clear();

	// Find the circle center
	UnitNormal	 = RootComponent->GetUpVector().Normalized();
	CircleCenter = RootComponent->GetWorldLocation() + UnitNormal * ConeHeight;

	FVector U = RootComponent->GetForwardVector().Normalized();
	FVector V = RootComponent->GetRightVector().Normalized();

	// Find the unit orthogonal. We can choose between U or V. Not used for now
	UnitOrthogonal = U;

	// Find the sphere vertices
	//float Circumference = 2 * M_PI * Radius;
	float angle = 2 * M_PI / NumCircleVertex;
	for (size_t i = 0; i < NumCircleVertex; i++) {
		FVector Point = CircleCenter + (U * cosf(angle * i) + V * sinf(angle * i)) * Radius;
		CircleVertices.push_back(Point);
	}
}

void ASpotlight::AddConeLinesToBatch(FBatchedLine* BatchLine) {
	if (!BatchLine || CircleVertices.empty()) {
		return;
	}

	FVector ConeApex = RootComponent->GetWorldLocation();
	for (size_t i = 0; i < NumCircleVertex; i++) {
		// Point to Apex
		BatchLine->AddLine(CircleVertices[i], ConeApex, ConeColor);
		
		// Point to next Point
		BatchLine->AddLine(CircleVertices[i], CircleVertices[(i + 1) % NumCircleVertex], ConeColor);
	}
}

void ASpotlight::Tick(float DeltaTIme) {
	if (bIsConeDirty) {
		UpdateCone();
	}
}