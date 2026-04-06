#include "Gizmo.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "Camera/Camera.h"
#include "Math/Transform.h"
#include "UnrealEditorStyledGizmo.h"
#include "Graphics/D3D11/D3D11RHI.h"
#include "Graphics/D3D11/D3D11Utils.h"

namespace
{
	constexpr float ParallelTolerance = 1.0e-6f;
	constexpr float MinGizmoScale = 0.05f;
	constexpr float MaxGizmoScale = 3.0f;
	constexpr float GizmoViewportHeightRatio = 0.15f;
	constexpr float TranslationAxisLengthUnits = 47.0f;
	constexpr float ScaleAxisLengthUnits = 25.0f;
	constexpr float ScaleReferenceUnits = 20.0f;
	constexpr float UniformScalePixelsPerUnit = 120.0f;
	constexpr float MinScaleMagnitude = 0.01f;
	constexpr float MaxScaleMagnitude = 1000.0f;
	constexpr float PI_CONST = 3.1415926535897932f;

	float ClampScaleComponent(float Value)
	{
		float ClampedValue = std::clamp(Value, -MaxScaleMagnitude, MaxScaleMagnitude);
		if (std::abs(ClampedValue) < MinScaleMagnitude)
		{
			ClampedValue = (ClampedValue < 0.0f) ? -MinScaleMagnitude : MinScaleMagnitude;
		}
		return ClampedValue;
	}

	FVector ClampScaleVector(const FVector& InScale)
	{
		return FVector(ClampScaleComponent(InScale.X), ClampScaleComponent(InScale.Y), ClampScaleComponent(InScale.Z));
	}

	bool IntersectPlane(const FRay& Ray, const FVector& PlaneOrigin, const FVector& PlaneNormal, FVector& OutIntersection)
	{
		const float Denominator = FVector::DotProduct(PlaneNormal, Ray.Direction);
		if (std::abs(Denominator) <= ParallelTolerance) return false;
		const float RayParameter = FVector::DotProduct(PlaneOrigin - Ray.Origin, PlaneNormal) / Denominator;
		if (RayParameter < 0.0f) return false;
		OutIntersection = Ray.Origin + Ray.Direction * RayParameter;
		return true;
	}

	bool RayTriangleIntersectTwoSided(const FRay& Ray, const FVector& V0, const FVector& V1, const FVector& V2, float& OutDistance)
	{
		const FVector Edge1 = V1 - V0; const FVector Edge2 = V2 - V0; const FVector H = FVector::CrossProduct(Ray.Direction, Edge2);
		const float A = FVector::DotProduct(Edge1, H); if (std::abs(A) <= ParallelTolerance) return false;
		const float F = 1.0f / A; const FVector S = Ray.Origin - V0; const float U = F * FVector::DotProduct(S, H);
		if (U < 0.0f || U > 1.0f) return false;
		const FVector Q = FVector::CrossProduct(S, Edge1); const float V = F * FVector::DotProduct(Ray.Direction, Q);
		if (V < 0.0f || U + V > 1.0f) return false;
		const float T = F * FVector::DotProduct(Edge2, Q); if (T <= ParallelTolerance) return false;
		OutDistance = T; return true;
	}
}

struct FGizmoMeshBuffer
{
	Mesh CpuMesh;
	TComPtr<ID3D11Buffer> VertexBuffer;
	TComPtr<ID3D11Buffer> IndexBuffer;
	UINT IndexCount = 0;

	bool Initialize(ID3D11Device* Device, const Mesh& InMesh)
	{
		CpuMesh = InMesh;
		if (CpuMesh.vertices.empty() || CpuMesh.indices.empty()) return false;
		if (!D3D11Utils::CreateImmutableBuffer(Device, static_cast<UINT>(sizeof(Vertex) * CpuMesh.vertices.size()), D3D11_BIND_VERTEX_BUFFER, CpuMesh.vertices.data(), VertexBuffer)) return false;
		if (!D3D11Utils::CreateImmutableBuffer(Device, static_cast<UINT>(sizeof(uint32_t) * CpuMesh.indices.size()), D3D11_BIND_INDEX_BUFFER, CpuMesh.indices.data(), IndexBuffer)) return false;
		IndexCount = static_cast<UINT>(CpuMesh.indices.size());
		return true;
	}
};

struct FGizmo::FGizmoRenderResources
{
	TComPtr<ID3D11VertexShader> VertexShader;
	TComPtr<ID3D11PixelShader> PixelShader;
	TComPtr<ID3D11InputLayout> InputLayout;
	TComPtr<ID3D11Buffer> ConstantBuffer;

	FGizmoMeshBuffer TransAxisX, TransAxisY, TransAxisZ;
	FGizmoMeshBuffer TransPlaneXY, TransPlaneXZ, TransPlaneYZ;
	FGizmoMeshBuffer TransScreen;

	FGizmoMeshBuffer RotRingX, RotRingY, RotRingZ, RotScreen;
	FGizmoMeshBuffer ScaleAxisX, ScaleAxisY, ScaleAxisZ;
	FGizmoMeshBuffer ScalePlaneXY, ScalePlaneXZ, ScalePlaneYZ, ScaleCenter;

	bool bShadersInitialized = false;
	bool bTranslationInitialized = false;
	bool bRotationInitialized = false;
	bool bScaleInitialized = false;
};

struct alignas(16) FGizmoConstants
{
	FMatrix WorldViewProjection = FMatrix::Identity;
	FVector4 HighlightTint = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
};

FGizmo::FGizmo() : RenderResources(std::make_unique<FGizmoRenderResources>()) {}
FGizmo::~FGizmo() = default;

void FGizmo::SetMode(EGizmoMode InMode) { Mode = InMode; ClearHover(); EndDrag(); }
void FGizmo::SetCoordinateSpace(EGizmoCoordinateSpace InSpace) { if (CoordinateSpace != InSpace) { CoordinateSpace = InSpace; ClearHover(); EndDrag(); } }
void FGizmo::ToggleCoordinateSpace() { SetCoordinateSpace(CoordinateSpace == EGizmoCoordinateSpace::World ? EGizmoCoordinateSpace::Local : EGizmoCoordinateSpace::World); }
void FGizmo::CycleMode() { Mode = static_cast<EGizmoMode>((static_cast<uint8>(Mode) + 1) % 3); ClearHover(); EndDrag(); }

bool FGizmo::EnsureTranslationMeshes(const FD3D11RHI& InRHI) const
{
	if (RenderResources->bTranslationInitialized) return true;
	ID3D11Device* Device = InRHI.GetDevice();
	TranslationDesc Desc{}; TranslationGizmo RawGizmo = GenerateTranslationGizmo(Desc);
	RenderResources->bTranslationInitialized =
		RenderResources->TransAxisX.Initialize(Device, RawGizmo.axisX) && RenderResources->TransAxisY.Initialize(Device, RawGizmo.axisY) && RenderResources->TransAxisZ.Initialize(Device, RawGizmo.axisZ) &&
		RenderResources->TransPlaneXY.Initialize(Device, RawGizmo.planeXY) && RenderResources->TransPlaneXZ.Initialize(Device, RawGizmo.planeXZ) && RenderResources->TransPlaneYZ.Initialize(Device, RawGizmo.planeYZ) &&
		RenderResources->TransScreen.Initialize(Device, RawGizmo.screenSphere);
	return RenderResources->bTranslationInitialized;
}

bool FGizmo::EnsureRotationMeshes(const FD3D11RHI& InRHI, const FCamera* Camera, const FVector& GizmoWorldLocation) const
{
	if (!Camera) return false;
	const RotationDesc Desc = BuildRotationDesc(Camera, GizmoWorldLocation);
	const bool bViewChanged = !CachedRotationCameraDirection.Equals(Desc.cameraDirection, 1.0e-4f) || !CachedRotationViewUp.Equals(Desc.viewUp, 1.0e-4f) || !CachedRotationViewRight.Equals(Desc.viewRight, 1.0e-4f);
	const bool bShapeStateChanged = (CachedRotationDragging != Desc.dragging) || (CachedRotationActiveAxis != ActiveAxis) || Desc.dragging;

	if (bViewChanged || bShapeStateChanged || !RenderResources->bRotationInitialized)
	{
		RotationGizmo RawGizmo = GenerateRotationGizmo(Desc);
		ID3D11Device* Device = InRHI.GetDevice();
		RenderResources->bRotationInitialized =
			RenderResources->RotRingX.Initialize(Device, RawGizmo.ringX) && RenderResources->RotRingY.Initialize(Device, RawGizmo.ringY) && RenderResources->RotRingZ.Initialize(Device, RawGizmo.ringZ) && RenderResources->RotScreen.Initialize(Device, RawGizmo.screenRing);
		CachedRotationCameraDirection = Desc.cameraDirection; CachedRotationViewUp = Desc.viewUp; CachedRotationViewRight = Desc.viewRight; CachedRotationDragging = Desc.dragging; CachedRotationActiveAxis = ActiveAxis;
	}
	return RenderResources->bRotationInitialized;
}

bool FGizmo::EnsureScaleMeshes(const FD3D11RHI& InRHI) const
{
	if (RenderResources->bScaleInitialized) return true;
	ID3D11Device* Device = InRHI.GetDevice();
	ScaleDesc Desc{}; ScaleGizmo RawGizmo = GenerateScaleGizmo(Desc);
	RenderResources->bScaleInitialized =
		RenderResources->ScaleAxisX.Initialize(Device, RawGizmo.axisX) && RenderResources->ScaleAxisY.Initialize(Device, RawGizmo.axisY) && RenderResources->ScaleAxisZ.Initialize(Device, RawGizmo.axisZ) &&
		RenderResources->ScalePlaneXY.Initialize(Device, RawGizmo.planeXY) && RenderResources->ScalePlaneXZ.Initialize(Device, RawGizmo.planeXZ) && RenderResources->ScalePlaneYZ.Initialize(Device, RawGizmo.planeYZ) &&
		RenderResources->ScaleCenter.Initialize(Device, RawGizmo.centerCube);
	return RenderResources->bScaleInitialized;
}

FVector FGizmo::GetTargetLocation(const FMatrix* TargetMatrix) const { return TargetMatrix ? FVector(TargetMatrix->M[3][0], TargetMatrix->M[3][1], TargetMatrix->M[3][2]) : FVector::ZeroVector; }
FVector FGizmo::GetAxisVector(EGizmoAxis Axis) { switch (Axis) { case EGizmoAxis::X: return FVector::ForwardVector; case EGizmoAxis::Y: return FVector::RightVector; case EGizmoAxis::Z: return FVector::UpVector; default: return FVector::ZeroVector; } }
FVector FGizmo::GetPlaneNormal(EGizmoAxis Axis) { switch (Axis) { case EGizmoAxis::XY: return FVector::UpVector; case EGizmoAxis::XZ: return FVector::RightVector; case EGizmoAxis::YZ: return FVector::ForwardVector; default: return FVector::ZeroVector; } }

FVector FGizmo::GetGizmoAxisVector(EGizmoAxis Axis, const FMatrix* TargetMatrix) const
{
	FVector WorldAxis = GetAxisVector(Axis);
	if ((Mode == EGizmoMode::Scale || CoordinateSpace == EGizmoCoordinateSpace::Local) && TargetMatrix && !WorldAxis.IsNearlyZero(ParallelTolerance))
		return FTransform(*TargetMatrix).GetRotation().RotateVector(WorldAxis).GetSafeNormal();
 return WorldAxis;
}

FVector FGizmo::GetGizmoPlaneNormal(EGizmoAxis Axis, const FMatrix* TargetMatrix) const
{
 FVector WorldNormal = GetPlaneNormal(Axis);
 if ((Mode == EGizmoMode::Scale || CoordinateSpace == EGizmoCoordinateSpace::Local) && TargetMatrix && !WorldNormal.IsNearlyZero(ParallelTolerance))
	 return FTransform(*TargetMatrix).GetRotation().RotateVector(WorldNormal).GetSafeNormal();
 return WorldNormal;
}

EGizmoAxis FGizmo::GetDisplayAxis() const { return ActiveAxis != EGizmoAxis::None ? ActiveAxis : HoveredAxis; }

float FGizmo::ComputeGizmoScale(const FVector& WorldPosition, const FCamera* Camera) const
{
 if (!Camera) return MinGizmoScale;
 float Distance = (WorldPosition - Camera->GetLocation()).Size();
 float HalfFovRadians = (Camera->GetFOV() * 0.5f) * PI_CONST / 180.0f;
 float VisibleHeight = 2.0f * (std::max)(Distance, 1.0f) * std::tan(HalfFovRadians);
 return std::clamp(VisibleHeight * GizmoViewportHeightRatio / ((Mode == EGizmoMode::Scale) ? ScaleAxisLengthUnits : TranslationAxisLengthUnits), MinGizmoScale, MaxGizmoScale);
}

float FGizmo::GetRenderGizmoScale(float BaseGizmoScale) const { return (Mode == EGizmoMode::Scale) ? (BaseGizmoScale * 0.5f) : BaseGizmoScale; }

RotationDesc FGizmo::BuildRotationDesc(const FCamera* Camera, const FVector& GizWorldLocation) const
{
 RotationDesc Desc{};
 if (!Camera) return Desc;

 FMatrix InvView = Camera->GetViewMatrix().GetInverse();
 FVector CamRight(InvView.M[0][0], InvView.M[0][1], InvView.M[0][2]);
 FVector CamUp(InvView.M[1][0], InvView.M[1][1], InvView.M[1][2]);
 FVector CamFwd(InvView.M[2][0], InvView.M[2][1], InvView.M[2][2]);

 Desc.cameraDirection = (GizWorldLocation - Camera->GetLocation()).GetSafeNormal();
 if (Desc.cameraDirection.IsNearlyZero(ParallelTolerance)) Desc.cameraDirection = CamFwd.GetSafeNormal();
 Desc.viewUp = CamUp.GetSafeNormal();
 Desc.viewRight = CamRight.GetSafeNormal();
 Desc.includeScreenRing = true;
 Desc.dragging = (Mode == EGizmoMode::Rotation && ActiveAxis != EGizmoAxis::None);
 Desc.deltaRotationDegrees = CurrentRotationDeltaDegrees;
 Desc.activeAxis = static_cast<AxisId>(ActiveAxis);
 return Desc;
}

EGizmoAxis FGizmo::HitTestAxis(const FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray) const
{
 if (!TargetMatrix || !InCamera) return EGizmoAxis::None;

 const FVector WorldLocation = GetTargetLocation(TargetMatrix);
 const float GizmoScale = GetRenderGizmoScale(ComputeGizmoScale(WorldLocation, InCamera));
 const FQuat GizmoRot = (Mode == EGizmoMode::Scale || CoordinateSpace == EGizmoCoordinateSpace::Local) ? FTransform(*TargetMatrix).GetRotation() : FQuat::Identity;
 const FMatrix AxisGizmoWorld = FTransform(GizmoRot, WorldLocation, FVector(GizmoScale, GizmoScale, GizmoScale)).ToMatrixWithScale();
 const FMatrix ScreenGizmoWorld = FTransform(FQuat::Identity, WorldLocation, FVector(GizmoScale, GizmoScale, GizmoScale)).ToMatrixWithScale();

 EGizmoAxis BestAxis = EGizmoAxis::None;
 float BestDistance = (std::numeric_limits<float>::max)();

 auto TestMesh = [&](const FGizmoMeshBuffer& Buffer, const FMatrix& MeshWorld, EGizmoAxis Handle) {
	 const auto& Indices = Buffer.CpuMesh.indices; const auto& Vertices = Buffer.CpuMesh.vertices;
	 if (Indices.empty()) return;
	 for (size_t i = 0; i + 2 < Indices.size(); i += 3) {
		 const FVector V0 = MeshWorld.TransformPosition(Vertices[Indices[i]].position);
		 const FVector V1 = MeshWorld.TransformPosition(Vertices[Indices[i + 1]].position);
		 const FVector V2 = MeshWorld.TransformPosition(Vertices[Indices[i + 2]].position);
		 float HitDist = 0.0f;
		 if (RayTriangleIntersectTwoSided(Ray, V0, V1, V2, HitDist) && HitDist < BestDistance) { BestDistance = HitDist; BestAxis = Handle; }
	 }
 };

 if (Mode == EGizmoMode::Location) {
	 TestMesh(RenderResources->TransAxisX, AxisGizmoWorld, EGizmoAxis::X); TestMesh(RenderResources->TransAxisY, AxisGizmoWorld, EGizmoAxis::Y); TestMesh(RenderResources->TransAxisZ, AxisGizmoWorld, EGizmoAxis::Z);
	 TestMesh(RenderResources->TransPlaneXY, AxisGizmoWorld, EGizmoAxis::XY); TestMesh(RenderResources->TransPlaneXZ, AxisGizmoWorld, EGizmoAxis::XZ); TestMesh(RenderResources->TransPlaneYZ, AxisGizmoWorld, EGizmoAxis::YZ);
	 TestMesh(RenderResources->TransScreen, AxisGizmoWorld, EGizmoAxis::Screen);
 }
 else if (Mode == EGizmoMode::Rotation) {
	 TestMesh(RenderResources->RotRingX, AxisGizmoWorld, EGizmoAxis::X); TestMesh(RenderResources->RotRingY, AxisGizmoWorld, EGizmoAxis::Y); TestMesh(RenderResources->RotRingZ, AxisGizmoWorld, EGizmoAxis::Z);
	 TestMesh(RenderResources->RotScreen, ScreenGizmoWorld, EGizmoAxis::Screen);
 }
 else if (Mode == EGizmoMode::Scale) {
	 TestMesh(RenderResources->ScaleAxisX, AxisGizmoWorld, EGizmoAxis::X); TestMesh(RenderResources->ScaleAxisY, AxisGizmoWorld, EGizmoAxis::Y); TestMesh(RenderResources->ScaleAxisZ, AxisGizmoWorld, EGizmoAxis::Z);
	 TestMesh(RenderResources->ScalePlaneXY, AxisGizmoWorld, EGizmoAxis::XY); TestMesh(RenderResources->ScalePlaneXZ, AxisGizmoWorld, EGizmoAxis::XZ); TestMesh(RenderResources->ScalePlaneYZ, AxisGizmoWorld, EGizmoAxis::YZ);
	 TestMesh(RenderResources->ScaleCenter, AxisGizmoWorld, EGizmoAxis::XYZ);
 }
 return BestAxis;
}

bool FGizmo::BeginDrag(FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray, int32 ScreenX, int32 ScreenY)
{
 if (!TargetMatrix || !InCamera) return false;
 ActiveAxis = HitTestAxis(TargetMatrix, InCamera, Ray);
 if (ActiveAxis == EGizmoAxis::None) return false;

 DragStartMatrix = *TargetMatrix;
 DragStartGizmoLocation = GetTargetLocation(TargetMatrix);

 if (Mode == EGizmoMode::Location) return BeginTranslationDrag(ActiveAxis, TargetMatrix, InCamera, Ray);
 if (Mode == EGizmoMode::Rotation) return BeginRotationDrag(ActiveAxis, TargetMatrix, InCamera, Ray);
 if (Mode == EGizmoMode::Scale) return BeginScaleDrag(ActiveAxis, TargetMatrix, InCamera, Ray, ScreenX, ScreenY);
 return false;
}

bool FGizmo::BeginTranslationDrag(EGizmoAxis AxisId, FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray)
{
 const FVector Axis = GetGizmoAxisVector(AxisId, TargetMatrix);
 FMatrix InvView = InCamera->GetViewMatrix().GetInverse();
 FVector CamRight = FVector(InvView.M[0][0], InvView.M[0][1], InvView.M[0][2]).GetSafeNormal();
 FVector CamFwd = FVector(InvView.M[2][0], InvView.M[2][1], InvView.M[2][2]).GetSafeNormal();

 if (AxisId >= EGizmoAxis::X && AxisId <= EGizmoAxis::Z) {
	 FVector Tangent = FVector::CrossProduct(CamFwd, Axis);
	 if (Tangent.SizeSquared() <= ParallelTolerance) Tangent = FVector::CrossProduct(CamRight, Axis);
	 DragPlaneNormal = FVector::CrossProduct(Axis, Tangent).GetSafeNormal();
 }
 else if (AxisId == EGizmoAxis::Screen) { DragPlaneNormal = CamFwd; }
 else { DragPlaneNormal = GetGizmoPlaneNormal(AxisId, TargetMatrix); }

 if (!IntersectPlane(Ray, DragStartGizmoLocation, DragPlaneNormal, DragStartIntersection)) return false;
 DragStartAxisDistance = (AxisId >= EGizmoAxis::X && AxisId <= EGizmoAxis::Z) ? FVector::DotProduct(DragStartIntersection - DragStartGizmoLocation, Axis) : 0.0f;
 return true;
}

bool FGizmo::BeginRotationDrag(EGizmoAxis AxisId, FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray)
{
 FMatrix InvView = InCamera->GetViewMatrix().GetInverse();
 FVector CamFwd = FVector(InvView.M[2][0], InvView.M[2][1], InvView.M[2][2]).GetSafeNormal();

 DragPlaneNormal = (AxisId == EGizmoAxis::Screen) ? CamFwd : GetGizmoAxisVector(AxisId, TargetMatrix);
 FVector Intersection; if (!IntersectPlane(Ray, DragStartGizmoLocation, DragPlaneNormal, Intersection)) return false;
 DragStartRotationVector = (Intersection - DragStartGizmoLocation).GetSafeNormal();
 if (DragStartRotationVector.IsNearlyZero(ParallelTolerance)) return false;
 return true;
}

bool FGizmo::BeginScaleDrag(EGizmoAxis AxisId, FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray, int32 ScreenX, int32 ScreenY)
{
 FMatrix InvView = InCamera->GetViewMatrix().GetInverse();
 FVector CamRight = FVector(InvView.M[0][0], InvView.M[0][1], InvView.M[0][2]).GetSafeNormal();
 FVector CamFwd = FVector(InvView.M[2][0], InvView.M[2][1], InvView.M[2][2]).GetSafeNormal();

 if (AxisId >= EGizmoAxis::X && AxisId <= EGizmoAxis::Z) {
	 FVector Tangent = FVector::CrossProduct(CamFwd, GetGizmoAxisVector(AxisId, TargetMatrix));
	 if (Tangent.SizeSquared() <= ParallelTolerance) Tangent = FVector::CrossProduct(CamRight, GetGizmoAxisVector(AxisId, TargetMatrix));
	 DragPlaneNormal = FVector::CrossProduct(GetGizmoAxisVector(AxisId, TargetMatrix), Tangent).GetSafeNormal();
 }
 else if (AxisId >= EGizmoAxis::XY && AxisId <= EGizmoAxis::YZ) { DragPlaneNormal = GetGizmoPlaneNormal(AxisId, TargetMatrix); }
 else { DragPlaneNormal = CamFwd; }

 FVector Intersection; if (!IntersectPlane(Ray, DragStartGizmoLocation, DragPlaneNormal, Intersection)) return false;
 DragStartIntersection = Intersection; DragStartActorScale = FTransform(DragStartMatrix).GetScale3D();
 DragStartAxisDistance = (AxisId >= EGizmoAxis::X && AxisId <= EGizmoAxis::Z) ? FVector::DotProduct(Intersection - DragStartGizmoLocation, GetGizmoAxisVector(AxisId, TargetMatrix)) : 0.0f;
 DragStartScreenX = ScreenX; DragStartScreenY = ScreenY;
 return true;
}

bool FGizmo::UpdateDrag(FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray, int32 ScreenX, int32 ScreenY)
{
 if (ActiveAxis == EGizmoAxis::None || !TargetMatrix || !InCamera) return false;
 FVector Intersection; if (!IntersectPlane(Ray, DragStartGizmoLocation, DragPlaneNormal, Intersection)) return false;

 FTransform CurrentTransform(DragStartMatrix);

 if (Mode == EGizmoMode::Location)
 {
	 FVector NewLoc = DragStartGizmoLocation;
	 if (ActiveAxis >= EGizmoAxis::X && ActiveAxis <= EGizmoAxis::Z) NewLoc += GetGizmoAxisVector(ActiveAxis, &DragStartMatrix) * (FVector::DotProduct(Intersection - DragStartGizmoLocation, GetGizmoAxisVector(ActiveAxis, &DragStartMatrix)) - DragStartAxisDistance);
	 else NewLoc += (Intersection - DragStartIntersection);
	 CurrentTransform.SetLocation(NewLoc);
 }
 else if (Mode == EGizmoMode::Rotation)
 {
	 const FVector CurrentVec = (Intersection - DragStartGizmoLocation).GetSafeNormal();
	 if (CurrentVec.IsNearlyZero(ParallelTolerance)) return false;
	 const float SignedAngle = std::atan2(FVector::DotProduct(FVector::CrossProduct(DragStartRotationVector, CurrentVec), DragPlaneNormal), FVector::DotProduct(DragStartRotationVector, CurrentVec));
	 CurrentRotationDeltaDegrees = SignedAngle * 180.0f / PI_CONST;
	 CurrentTransform.SetRotation((CurrentTransform.GetRotation() * FQuat(DragPlaneNormal, SignedAngle)).GetNormalized());
 }
 else if (Mode == EGizmoMode::Scale)
 {
	 FVector NewScale = DragStartActorScale;
	 const float Denom = std::max(ScaleReferenceUnits * GetRenderGizmoScale(ComputeGizmoScale(DragStartGizmoLocation, InCamera)), ParallelTolerance);

	 if (ActiveAxis >= EGizmoAxis::X && ActiveAxis <= EGizmoAxis::Z) {
		 int AxisIdx = static_cast<int>(ActiveAxis) - 1;
		 NewScale[AxisIdx] = ClampScaleComponent(DragStartActorScale[AxisIdx] + (FVector::DotProduct(Intersection - DragStartGizmoLocation, GetGizmoAxisVector(ActiveAxis, &DragStartMatrix)) - DragStartAxisDistance) / Denom);
	 }
	 else if (ActiveAxis == EGizmoAxis::XYZ) {
		 const float UniformDelta = static_cast<float>((ScreenX - DragStartScreenX) - (ScreenY - DragStartScreenY)) / UniformScalePixelsPerUnit;
		 NewScale = ClampScaleVector(DragStartActorScale + FVector(UniformDelta, UniformDelta, UniformDelta));
	 }
	 CurrentTransform.SetScale3D(NewScale);
 }

 *TargetMatrix = CurrentTransform.ToMatrixWithScale();
 return true;
}

void FGizmo::UpdateHover(const FMatrix* TargetMatrix, FCamera* InCamera, const FRay& Ray)
{
 if (IsDragging()) return;
 HoveredAxis = HitTestAxis(TargetMatrix, InCamera, Ray);
}

void FGizmo::ClearHover() { HoveredAxis = EGizmoAxis::None; }
void FGizmo::EndDrag() { ActiveAxis = EGizmoAxis::None; CurrentRotationDeltaDegrees = 0.0f; DragStartMatrix = FMatrix::Identity; }

void FGizmo::Render(const FD3D11RHI& InRHI, const FCamera& InCamera, const FMatrix& TargetWorldMatrix) const
{
	ID3D11DeviceContext* DC = InRHI.GetDeviceContext();
	ID3D11Device* Device = InRHI.GetDevice();
	if (!DC || !Device) return;

	if (!RenderResources->bShadersInitialized)
	{
		static constexpr char ShaderSrc[] = R"(
		cbuffer CB : register(b0) { row_major float4x4 WVP; float4 Tint; };
		struct VSIn { float3 Pos:POSITION; float4 Col:COLOR; float3 Norm:NORMAL; float2 UV:TEXCOORD; };
		struct VSOut{ float4 Pos:SV_POSITION; float4 Col:COLOR; };
		VSOut VSMain(VSIn I) { VSOut O; O.Pos = mul(float4(I.Pos, 1.0), WVP); O.Col = I.Col * Tint; return O; }
		float4 PSMain(VSOut I) : SV_TARGET { return I.Col; }
	)";
	 TComPtr<ID3DBlob> VS, PS;
	 D3D11Utils::CompileShaderFromSource(ShaderSrc, "VSMain", "vs_5_0", VS, "GizmoVS");
	 D3D11Utils::CompileShaderFromSource(ShaderSrc, "PSMain", "ps_5_0", PS, "GizmoPS");
	 Device->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), nullptr, RenderResources->VertexShader.GetAddressOf());
	 Device->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), nullptr, RenderResources->PixelShader.GetAddressOf());
	 const D3D11_INPUT_ELEMENT_DESC Layout[] = {
		 {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		 {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		 {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		 {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0}
	 };
	 Device->CreateInputLayout(Layout, 4, VS->GetBufferPointer(), VS->GetBufferSize(), RenderResources->InputLayout.GetAddressOf());
	 D3D11Utils::CreateDynamicConstantBuffer(Device, sizeof(FGizmoConstants), RenderResources->ConstantBuffer);
	 RenderResources->bShadersInitialized = true;
 }

 if (Mode == EGizmoMode::Location) EnsureTranslationMeshes(InRHI);
 else if (Mode == EGizmoMode::Rotation) EnsureRotationMeshes(InRHI, &InCamera, GetTargetLocation(&TargetWorldMatrix));
 else if (Mode == EGizmoMode::Scale) EnsureScaleMeshes(InRHI);

 DC->RSSetState(InRHI.GetRasterizerState(D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE));
 DC->OMSetDepthStencilState(InRHI.GetDepthStencilState(FALSE, D3D11_DEPTH_WRITE_MASK_ZERO, D3D11_COMPARISON_ALWAYS), 0);
 DC->OMSetBlendState(InRHI.GetBlendState(TRUE, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL), nullptr, 0xffffffffu);

 DC->IASetInputLayout(RenderResources->InputLayout.Get());
 DC->VSSetShader(RenderResources->VertexShader.Get(), nullptr, 0);
 DC->PSSetShader(RenderResources->PixelShader.Get(), nullptr, 0);

 const float GizmoScale = GetRenderGizmoScale(ComputeGizmoScale(GetTargetLocation(&TargetWorldMatrix), &InCamera));
 const FQuat GizmoRot = (Mode == EGizmoMode::Scale || CoordinateSpace == EGizmoCoordinateSpace::Local) ? FTransform(TargetWorldMatrix).GetRotation() : FQuat::Identity;
 const FMatrix AxisWorld = FTransform(GizmoRot, GetTargetLocation(&TargetWorldMatrix), FVector(GizmoScale, GizmoScale, GizmoScale)).ToMatrixWithScale();
 const FMatrix ScreenWorld = FTransform(FQuat::Identity, GetTargetLocation(&TargetWorldMatrix), FVector(GizmoScale, GizmoScale, GizmoScale)).ToMatrixWithScale();

 auto DrawPart = [&](const FGizmoMeshBuffer& Buffer, const FMatrix& WorldMat, EGizmoAxis AxisId) {
	 if (!Buffer.VertexBuffer) return;
	 FGizmoConstants CB;
	 CB.WorldViewProjection = WorldMat * InCamera.GetViewMatrix() * InCamera.GetProjectionMatrix();
	 CB.HighlightTint = (GetDisplayAxis() == AxisId && AxisId != EGizmoAxis::None) ? FVector4(1.0f, 1.0f, 0.0f, 1.0f) : FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	 D3D11Utils::UpdateDynamicBuffer(DC, RenderResources->ConstantBuffer.Get(), CB);
	 ID3D11Buffer* CBs[] = { RenderResources->ConstantBuffer.Get() };
	 DC->VSSetConstantBuffers(0, 1, CBs);

	 UINT Stride = sizeof(Vertex), Offset = 0;
	 ID3D11Buffer* VBs[] = { Buffer.VertexBuffer.Get() };
	 DC->IASetVertexBuffers(0, 1, VBs, &Stride, &Offset);
	 DC->IASetIndexBuffer(Buffer.IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	 DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	 DC->DrawIndexed(Buffer.IndexCount, 0, 0);
 };

 if (Mode == EGizmoMode::Location) {
	 DrawPart(RenderResources->TransAxisX, AxisWorld, EGizmoAxis::X); DrawPart(RenderResources->TransAxisY, AxisWorld, EGizmoAxis::Y); DrawPart(RenderResources->TransAxisZ, AxisWorld, EGizmoAxis::Z);
	 DrawPart(RenderResources->TransPlaneXY, AxisWorld, EGizmoAxis::XY); DrawPart(RenderResources->TransPlaneXZ, AxisWorld, EGizmoAxis::XZ); DrawPart(RenderResources->TransPlaneYZ, AxisWorld, EGizmoAxis::YZ);
	 DrawPart(RenderResources->TransScreen, AxisWorld, EGizmoAxis::Screen);
 }
 else if (Mode == EGizmoMode::Rotation) {
	 DrawPart(RenderResources->RotRingX, AxisWorld, EGizmoAxis::X); DrawPart(RenderResources->RotRingY, AxisWorld, EGizmoAxis::Y); DrawPart(RenderResources->RotRingZ, AxisWorld, EGizmoAxis::Z);
	 DrawPart(RenderResources->RotScreen, ScreenWorld, EGizmoAxis::Screen);
 }
 else if (Mode == EGizmoMode::Scale) {
	 DrawPart(RenderResources->ScaleAxisX, AxisWorld, EGizmoAxis::X); DrawPart(RenderResources->ScaleAxisY, AxisWorld, EGizmoAxis::Y); DrawPart(RenderResources->ScaleAxisZ, AxisWorld, EGizmoAxis::Z);
	 DrawPart(RenderResources->ScalePlaneXY, AxisWorld, EGizmoAxis::XY); DrawPart(RenderResources->ScalePlaneXZ, AxisWorld, EGizmoAxis::XZ); DrawPart(RenderResources->ScalePlaneYZ, AxisWorld, EGizmoAxis::YZ);
	 DrawPart(RenderResources->ScaleCenter, AxisWorld, EGizmoAxis::XYZ);
 }
}