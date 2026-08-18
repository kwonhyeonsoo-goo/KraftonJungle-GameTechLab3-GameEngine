#pragma once

#include <memory>
#include "Types/PlatformTypes.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Picking/PickingSystem.h" 

class FCamera;
class FD3D11RHI;
struct RotationDesc;

enum class EGizmoMode : uint8
{
	Location,
	Rotation,
	Scale
};

enum class EGizmoCoordinateSpace : uint8
{
	World,
	Local
};

enum class EGizmoAxis : uint8
{
	None = 0,
	X,
	Y,
	Z,
	XY,
	XZ,
	YZ,
	XYZ,
	Screen
};

class FGizmo
{
public:
	FGizmo();
	~FGizmo();

	void SetMode(EGizmoMode InMode);
	EGizmoMode GetMode() const { return Mode; }
	void CycleMode();

	void SetCoordinateSpace(EGizmoCoordinateSpace InSpace);
	EGizmoCoordinateSpace GetCoordinateSpace() const { return CoordinateSpace; }
	void ToggleCoordinateSpace();

	void Render(const FD3D11RHI& InRHI, const FCamera& InCamera, const FMatrix& TargetWorldMatrix) const;

	bool BeginDrag(FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray, int32 ScreenX, int32 ScreenY);
	bool UpdateDrag(FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray, int32 ScreenX, int32 ScreenY);
	void UpdateHover(const FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray);

	void ClearHover();
	void EndDrag();

	bool IsDragging() const { return ActiveAxis != EGizmoAxis::None; }
	EGizmoAxis HitTestAxis(const FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray) const;

private:
	bool EnsureTranslationMeshes(const FD3D11RHI& InRHI) const;
	bool EnsureRotationMeshes(const FD3D11RHI& InRHI, const FCamera* Camera, const FVector& GizmoWorldLocation) const;
	bool EnsureScaleMeshes(const FD3D11RHI& InRHI) const;

	bool BeginAxisDrag(EGizmoAxis Axis, FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray, int32 ScreenX, int32 ScreenY);
	bool BeginTranslationDrag(EGizmoAxis Axis, FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray);
	bool BeginRotationDrag(EGizmoAxis Axis, FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray);
	bool BeginScaleDrag(EGizmoAxis Axis, FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray, int32 ScreenX, int32 ScreenY);

	EGizmoAxis GetDisplayAxis() const;
	RotationDesc BuildRotationDesc(const FCamera* Camera, const FVector& GizmoWorldLocation) const;

	FVector GetTargetLocation(const FMatrix* TargetMatrix) const;
	FVector GetGizmoAxisVector(EGizmoAxis Axis, const FMatrix* TargetMatrix) const;
	FVector GetGizmoPlaneNormal(EGizmoAxis Axis, const FMatrix* TargetMatrix) const;

	static FVector GetAxisVector(EGizmoAxis Axis);
	static FVector GetPlaneNormal(EGizmoAxis Axis);

	float ComputeGizmoScale(const FVector& WorldPosition, const FCamera* Camera) const;
	float GetRenderGizmoScale(float BaseGizmoScale) const;

private:
	EGizmoMode Mode = EGizmoMode::Location;
	EGizmoCoordinateSpace CoordinateSpace = EGizmoCoordinateSpace::World;
	EGizmoAxis ActiveAxis = EGizmoAxis::None;
	EGizmoAxis HoveredAxis = EGizmoAxis::None;

	float CurrentRotationDeltaDegrees = 0.0f;
	FVector DragStartActorLocation = FVector::ZeroVector;
	FVector DragStartGizmoLocation = FVector::ZeroVector;
	FVector DragStartIntersection = FVector::ZeroVector;
	FVector DragPlaneNormal = FVector::ZeroVector;
	FVector DragStartRotationVector = FVector::ZeroVector;
	FVector DragStartActorScale = FVector(1.0f, 1.0f, 1.0f);
	float DragStartAxisDistance = 0.0f;
	FMatrix DragStartMatrix = FMatrix::Identity;
	int32 DragStartScreenX = 0;
	int32 DragStartScreenY = 0;

	mutable FVector CachedRotationCameraDirection = FVector::ZeroVector;
	mutable FVector CachedRotationViewUp = FVector::ZeroVector;
	mutable FVector CachedRotationViewRight = FVector::ZeroVector;
	mutable bool CachedRotationDragging = false;
	mutable EGizmoAxis CachedRotationActiveAxis = EGizmoAxis::None;

	struct FGizmoRenderResources;
	mutable std::unique_ptr<FGizmoRenderResources> RenderResources;
};