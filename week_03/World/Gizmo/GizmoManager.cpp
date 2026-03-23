#include "GizmoManager.h"
#include "Classes/AActor.h"
#include <cmath>

void FGizmoManager::Initialize(weak_ptr<UGizmoComponent> InComponent)
{
	Component = InComponent;
}

void FGizmoManager::SetHolding(bool bHold)
{
	bIsHolding = bHold;
	if (auto Comp = Component.lock())
		Comp->SetHoldingState(bHold);
}

void FGizmoManager::SetVisibility(bool bVisible)
{
	if (auto Comp = Component.lock())
		Comp->SetVisibility(bVisible);
}

void FGizmoManager::SetTarget(USceneComponent* NewTarget)
{
	if (!NewTarget) return;
	auto Comp = Component.lock();
	if (!Comp) return;

	TargetComponent = NewTarget;
	Comp->SetWorldLocation(TargetComponent->GetWorldLocation());
	UpdateGizmoTransform();
	Comp->SetVisibility(true);
}

void FGizmoManager::Deactivate()
{
	TargetComponent = nullptr;
	if (auto Comp = Component.lock())
		Comp->Deactivate();
}

void FGizmoManager::SetNextMode()
{
	UpdateGizmoMode(static_cast<EGizmoMode>((static_cast<int>(CurMode) + 1) % EGizmoMode::End));
}

void FGizmoManager::UpdateGizmoMode(EGizmoMode NewMode)
{
	CurMode = NewMode;
	UpdateGizmoTransform();
}

void FGizmoManager::UpdateGizmoTransform()
{
	auto Comp = Component.lock();
	if (!Comp || !TargetComponent) return;

	Comp->SetWorldLocation(TargetComponent->GetWorldLocation());
	Comp->UpdateMeshForMode(static_cast<int>(CurMode));

	switch (CurMode)
	{
	case Scale:
		Comp->SetRelativeRotation(TargetComponent->RelativeRotation);
		break;
	case Rotate:
	case Translate:
		Comp->SetRelativeRotation(bIsWorldSpace ? FVector() : TargetComponent->RelativeRotation);
		break;
	}
}

void FGizmoManager::SetTargetLocation(FVector NewLocation)
{
	if (!TargetComponent) return;
	TargetComponent->SetRelativeLocation(NewLocation);
	UpdateGizmoTransform();
}

void FGizmoManager::SetTargetRotation(FVector NewRotation)
{
	if (!TargetComponent) return;
	TargetComponent->SetRelativeRotation(NewRotation);
	UpdateGizmoTransform();
}

void FGizmoManager::SetTargetScale(FVector NewScale)
{
	if (!TargetComponent) return;
	FVector Safe = NewScale;
	if (Safe.X < 0.001f) Safe.X = 0.001f;
	if (Safe.Y < 0.001f) Safe.Y = 0.001f;
	if (Safe.Z < 0.001f) Safe.Z = 0.001f;
	TargetComponent->SetRelativeScale(Safe);
}

void FGizmoManager::ApplyScreenSpaceScaling(const FVector& CameraLocation)
{
	auto Comp = Component.lock();
	if (!Comp) return;
	float Distance = FVector::Distance(CameraLocation, Comp->GetWorldLocation());
	float NewScale = Distance * 0.1f;
	if (NewScale < 0.01f) NewScale = 0.01f;
	Comp->SetRelativeScale(FVector(NewScale, NewScale, NewScale));
}

void FGizmoManager::SetWorldSpace(bool bWorldSpace)
{
	bIsWorldSpace = bWorldSpace;
	UpdateGizmoTransform();
}

FVector FGizmoManager::GetVectorForAxis(int32 Axis)
{
	auto Comp = Component.lock();
	if (!Comp) return FVector(0, 0, 0);
	switch (Axis)
	{
	case 0: return Comp->GetForwardVector();
	case 1: return Comp->GetRightVector();
	case 2: return Comp->GetUpVector();
	default: return FVector(0, 0, 0);
	}
}

bool FGizmoManager::IntersectRayAxis(const FRay& Ray, FVector AxisEnd, float& OutRayT)
{
	auto Comp = Component.lock();
	if (!Comp) return false;

	FVector axisStart  = Comp->GetWorldLocation();
	FVector axisVector = AxisEnd - axisStart;
	FVector diffOrigin = Ray.Origin - axisStart;

	float RayDirDotRayDir = Ray.Direction.Dot(Ray.Direction);
	float RayDirDotAxis   = Ray.Direction.Dot(axisVector);
	float AxisDotAxis     = axisVector.Dot(axisVector);
	float RayDirDotDiff   = Ray.Direction.Dot(diffOrigin);
	float AxisDotDiff     = axisVector.Dot(diffOrigin);

	float Denominator = (RayDirDotRayDir * AxisDotAxis) - (RayDirDotAxis * RayDirDotAxis);
	float rayT, axisS;

	if (Denominator < 1e-6f)
	{
		rayT  = 0.0f;
		axisS = (AxisDotAxis > 0.0f) ? (AxisDotDiff / AxisDotAxis) : 0.0f;
	}
	else
	{
		rayT  = (RayDirDotAxis * AxisDotDiff - AxisDotAxis * RayDirDotDiff) / Denominator;
		axisS = (RayDirDotRayDir * AxisDotDiff - RayDirDotAxis * RayDirDotDiff) / Denominator;
	}

	if (rayT  < 0.0f) rayT  = 0.0f;
	if (axisS < 0.0f) axisS = 0.0f;
	else if (axisS > 1.0f) axisS = 1.0f;

	FVector closest  = Ray.Origin + (Ray.Direction * rayT);
	FVector onAxis   = axisStart  + (axisVector    * axisS);
	FVector diff     = closest - onAxis;
	float   distSq   = diff.Dot(diff);

	if (distSq < Radius * Radius)
	{
		OutRayT = rayT;
		return true;
	}
	return false;
}

void FGizmoManager::HandleDrag(float DragAmount)
{
	switch (CurMode)
	{
	case Translate: TranslateTarget(DragAmount); break;
	case Rotate:    RotateTarget(DragAmount);    break;
	case Scale:     ScaleTarget(DragAmount);     break;
	}
	UpdateGizmoTransform();
}

void FGizmoManager::TranslateTarget(float DragAmount)
{
	auto Comp = Component.lock();
	if (!Comp || !TargetComponent) return;
	FVector delta = GetVectorForAxis(Comp->GetSelectedAxis()) * DragAmount;
	Comp->AddWorldOffset(delta);
	TargetComponent->AddWorldOffset(delta);
	TargetComponent->GetOwningActor()->Transformed();
}

void FGizmoManager::RotateTarget(float DragAmount)
{
	auto Comp = Component.lock();
	if (!Comp || !TargetComponent) return;
	FMatrix curMatrix   = FMatrix::MakeRotationEuler(TargetComponent->RelativeRotation);
	FVector rotAxis     = GetVectorForAxis(Comp->GetSelectedAxis());
	FMatrix deltaMatrix = FMatrix::MakeRotationAxis(rotAxis, DragAmount);
	TargetComponent->SetRelativeRotation((curMatrix * deltaMatrix).GetEuler());
	TargetComponent->GetOwningActor()->Transformed();
}

void FGizmoManager::ScaleTarget(float DragAmount)
{
	auto Comp = Component.lock();
	if (!Comp || !TargetComponent) return;
	float scaleDelta = DragAmount * ScaleSensitivity;
	FVector NewScale = TargetComponent->RelativeScale3D;
	switch (Comp->GetSelectedAxis())
	{
	case 0: NewScale.X += scaleDelta; break;
	case 1: NewScale.Y += scaleDelta; break;
	case 2: NewScale.Z += scaleDelta; break;
	}
	TargetComponent->SetRelativeScale(NewScale);
	TargetComponent->GetOwningActor()->Transformed();
}

void FGizmoManager::UpdateLinearDrag(const FRay& Ray)
{
	auto Comp = Component.lock();
	if (!Comp) return;

	FVector axisVector = GetVectorForAxis(Comp->GetSelectedAxis());
	FVector planeNormal = axisVector.Cross(Ray.Direction);
	FVector projectDir  = planeNormal.Cross(axisVector);

	float Denom = Ray.Direction.Dot(projectDir);
	if (std::abs(Denom) < 1e-6f) return;

	FVector Current = Ray.Origin + (Ray.Direction * ((Comp->GetWorldLocation() - Ray.Origin).Dot(projectDir) / Denom));

	if (bIsFirstFrameOfDrag) { LastIntersectionLocation = Current; bIsFirstFrameOfDrag = false; return; }

	float DragAmount = (Current - LastIntersectionLocation).Dot(axisVector);
	HandleDrag(DragAmount);
	LastIntersectionLocation = Current;
}

void FGizmoManager::UpdateAngularDrag(const FRay& Ray)
{
	auto Comp = Component.lock();
	if (!Comp) return;

	FVector planeNormal = GetVectorForAxis(Comp->GetSelectedAxis());
	float Denom = Ray.Direction.Dot(planeNormal);
	if (std::abs(Denom) < 1e-6f) return;

	FVector Current = Ray.Origin + (Ray.Direction * ((Comp->GetWorldLocation() - Ray.Origin).Dot(planeNormal) / Denom));

	if (bIsFirstFrameOfDrag) { LastIntersectionLocation = Current; bIsFirstFrameOfDrag = false; return; }

	FVector CenterToLast    = (LastIntersectionLocation - Comp->GetWorldLocation()).Normalized();
	FVector CenterToCurrent = (Current                  - Comp->GetWorldLocation()).Normalized();

	float Dot   = Clamp(CenterToLast.Dot(CenterToCurrent), -1.0f, 1.0f);
	float Angle = std::acos(Dot);
	float Sign  = (CenterToLast.Cross(CenterToCurrent).Dot(planeNormal) >= 0.0f) ? 1.0f : -1.0f;

	HandleDrag(Sign * Angle);
	LastIntersectionLocation = Current;
}

void FGizmoManager::UpdateDrag(const FRay& Ray)
{
	auto Comp = Component.lock();
	if (!bIsHolding || !Comp || !Comp->IsActive()) return;
	if (Comp->GetSelectedAxis() == -1 || !TargetComponent) return;

	if (CurMode == Rotate)
		UpdateAngularDrag(Ray);
	else
		UpdateLinearDrag(Ray);
}

void FGizmoManager::DragEnd()
{
	bIsFirstFrameOfDrag = true;
	SetHolding(false);
}
