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

	// Base vectors
	FVector BaseNormal = RootComponent->GetUpVector().Normalized();
	FVector BaseRight = RootComponent->GetRightVector().Normalized();
	FVector BaseForward = RootComponent->GetForwardVector().Normalized();

	// Apply pitch around the local right axis, then yaw around the local up axis
	FMatrix PitchMat = FMatrix::MakeRotationAxis(BaseRight, Pitch);
	FMatrix YawMat = FMatrix::MakeRotationAxis(BaseForward, Yaw);
	FMatrix RotMat = PitchMat * YawMat;

	UnitNormal = (RotMat.TransformVector(BaseNormal)).Normalized();
	CircleCenter = RootComponent->GetWorldLocation() + UnitNormal * ConeHeight;

	// Recompute U and V perpendicular to the rotated axis
	FVector U = (RotMat.TransformVector(BaseForward)).Normalized();
	FVector V = UnitNormal.Cross(U).Normalized();

	// Find the unit orthogonal. We can choose between U or V. Not used for now
	UnitOrthogonal = U;

	float Angle = 2.0f * M_PI / NumCircleVertex;
	for (size_t i = 0; i < NumCircleVertex; i++) {
		FVector Point = CircleCenter + (U * cosf(Angle * i) + V * sinf(Angle * i)) * Radius;
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
		bIsConeDirty = false;
	}
}

//void ASpotlight::SetActorLocation(const FVector& Location, USceneComponent* WhichComp) {
//	if (WhichComp) {
//		WhichComp->SetRelativeLocation(Location);
//
//		if (WhichComp == RootComponent) {
//			bIsConeDirty = true;
//		}
//	}
//}
//void ASpotlight::SetActorRotation(const FVector& Rotation, USceneComponent* WhichComp) {
//	if (WhichComp) {
//		WhichComp->SetRelativeLocation(Rotation);
//
//		if (WhichComp == RootComponent) {
//			bIsConeDirty = true;
//		}
//	}
//}
//void ASpotlight::SetActorScale(const FVector& Scale, USceneComponent* WhichComp) {
//	if (WhichComp) {
//		WhichComp->SetRelativeScale(Scale);
//	}
//}