#include "PhysicsAssetPreviewComponent.h"

#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Math/MathUtils.h"
#include "Physics/PhysicsAsset.h"
#include "Physics/PhysicsAssetPreviewUtils.h"
#include "Render/Proxy/PhysicsAssetPreviewSceneProxy.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float MinShapeSize = 0.001f;
	constexpr int32 SphereSlices = 24;
	constexpr int32 SphereStacks = 12;
	constexpr int32 CapsuleSlices = 24;
	constexpr int32 CapsuleHemisphereStacks = 6;

	FTransform ComposePreviewDebugTransforms(const FTransform& ParentWorld, const FTransform& Local)
	{
		FTransform Result = Local;
		Result.Location = ParentWorld.Location + ParentWorld.Rotation.RotateVector(Local.Location);
		Result.Rotation = (ParentWorld.Rotation * Local.Rotation).GetNormalized();
		Result.Scale = FVector::OneVector;
		return Result;
	}

	FVector4 PhysicsPreviewShapeColor(bool bSelectedBody, bool bSelectedShape)
	{
		if (bSelectedShape)
		{
			return FVector4(1.0f, 0.96f, 0.47f, 0.50f);
		}

		if (bSelectedBody)
		{
			return FVector4(1.0f, 0.58f, 0.18f, 0.35f);
		}

		return FVector4(0.35f, 0.82f, 1.0f, 0.22f);
	}

	FVector ClampHalfExtent(FVector HalfExtent)
	{
		HalfExtent.X = (std::max)(HalfExtent.X, MinShapeSize);
		HalfExtent.Y = (std::max)(HalfExtent.Y, MinShapeSize);
		HalfExtent.Z = (std::max)(HalfExtent.Z, MinShapeSize);
		return HalfExtent;
	}
}

UPhysicsAssetPreviewComponent::UPhysicsAssetPreviewComponent()
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCastShadow(false);
}

UPhysicsAssetPreviewComponent::~UPhysicsAssetPreviewComponent()
{
	PreviewMeshBuffer.Release();
}

FPrimitiveSceneProxy* UPhysicsAssetPreviewComponent::CreateSceneProxy()
{
	return new FPhysicsAssetPreviewSceneProxy(this);
}

FMeshBuffer* UPhysicsAssetPreviewComponent::GetMeshBuffer() const
{
	return const_cast<FMeshBuffer*>(&PreviewMeshBuffer);
}

FMeshDataView UPhysicsAssetPreviewComponent::GetMeshDataView() const
{
	return FMeshDataView::FromMeshData(PreviewMeshData);
}

void UPhysicsAssetPreviewComponent::UpdateWorldAABB() const
{
	if (bHasPreviewBounds && CachedWorldBounds.IsValid())
	{
		WorldAABBMinLocation = CachedWorldBounds.Min;
		WorldAABBMaxLocation = CachedWorldBounds.Max;
		bWorldAABBDirty = false;
		bHasValidWorldAABB = true;
		return;
	}

	UPrimitiveComponent::UpdateWorldAABB();
}

void UPhysicsAssetPreviewComponent::UpdatePreview(
	UPhysicsAsset* InPhysicsAsset,
	USkeletalMeshComponent* InPreviewComponent,
	int32 InSelectedBodyIndex,
	int32 InSelectedShapeIndex,
	int32 InSelectedConstraintIndex,
	bool bInShowBodies,
	ID3D11Device* Device)
{
	PhysicsAsset = InPhysicsAsset;
	PreviewComponent = InPreviewComponent;
	SelectedBodyIndex = InSelectedBodyIndex;
	SelectedShapeIndex = InSelectedShapeIndex;
	SelectedConstraintIndex = InSelectedConstraintIndex;
	bShowBodies = bInShowBodies;

	if (!bShowBodies || !PhysicsAsset.Get() || !PreviewComponent.Get() || !Device)
	{
		ClearPreview(Device);
		return;
	}

	RebuildPreviewMesh();
	UploadPreviewMesh(Device);
	SetVisibility(!PreviewMeshData.Vertices.empty());
}

void UPhysicsAssetPreviewComponent::ClearPreview(ID3D11Device* Device)
{
	(void)Device;
	PreviewMeshData.Vertices.clear();
	PreviewMeshData.Indices.clear();
	CachedWorldBounds = FBoundingBox();
	bHasPreviewBounds = false;
	PreviewMeshBuffer.Release();

	SetVisibility(false);
	MarkWorldBoundsDirty();
	MarkProxyDirty(EDirtyFlag::Mesh);
}

void UPhysicsAssetPreviewComponent::RebuildPreviewMesh()
{
	PreviewMeshData.Vertices.clear();
	PreviewMeshData.Indices.clear();
	CachedWorldBounds = FBoundingBox();
	bHasPreviewBounds = false;

	UPhysicsAsset* Asset = PhysicsAsset.Get();
	USkeletalMeshComponent* MeshComponent = PreviewComponent.Get();
	if (!Asset || !MeshComponent || !bShowBodies)
	{
		return;
	}

	const TArray<FPhysicsAssetBodySetup>& Bodies = Asset->GetBodySetups();
	for (int32 BodyIndex = 0; BodyIndex < static_cast<int32>(Bodies.size()); ++BodyIndex)
	{
		FTransform BodyWorld;
		if (!FPhysicsAssetPreviewUtils::ComputePreviewBodyWorldTransform(
			MeshComponent,
			Asset,
			BodyIndex,
			BodyWorld))
		{
			continue;
		}

		const FPhysicsAssetBodySetup& Body = Bodies[BodyIndex];
		const bool bSelectedBody = SelectedBodyIndex == BodyIndex && SelectedConstraintIndex < 0;
		for (int32 ShapeIndex = 0; ShapeIndex < static_cast<int32>(Body.Shapes.size()); ++ShapeIndex)
		{
			const FPhysicsAssetShapeSetup& Shape = Body.Shapes[ShapeIndex];
			const FTransform ShapeWorld = ComposePreviewDebugTransforms(BodyWorld, Shape.LocalTransform);
			const bool bSelectedShape = bSelectedBody && SelectedShapeIndex == ShapeIndex;
			const FVector4 Color = PhysicsPreviewShapeColor(bSelectedBody, bSelectedShape);

			switch (Shape.Type)
			{
			case EPhysicsAssetShapeType::Box:
				AppendBox(ShapeWorld, ClampHalfExtent(Shape.BoxHalfExtent), Color);
				break;
			case EPhysicsAssetShapeType::Sphere:
				AppendSphere(ShapeWorld, (std::max)(Shape.SphereRadius, MinShapeSize), Color);
				break;
			case EPhysicsAssetShapeType::Capsule:
			{
				const float Radius = (std::max)(Shape.CapsuleRadius, MinShapeSize);
				const float HalfHeight = (std::max)(Shape.CapsuleHalfHeight, Radius);
				AppendCapsuleZAxis(ShapeWorld, Radius, HalfHeight, Color);
				break;
			}
			default:
				break;
			}
		}
	}
}

void UPhysicsAssetPreviewComponent::UploadPreviewMesh(ID3D11Device* Device)
{
	if (!Device)
	{
		return;
	}

	PreviewMeshBuffer.Create(Device, PreviewMeshData);
	MarkWorldBoundsDirty();
	MarkProxyDirty(EDirtyFlag::Mesh);
}

uint32 UPhysicsAssetPreviewComponent::AddVertexWorld(const FVector& WorldPosition, const FVector4& Color)
{
	const FVector LocalPosition = GetWorldInverseMatrix().TransformPositionWithW(WorldPosition);
	PreviewMeshData.Vertices.push_back({ LocalPosition, Color, 0 });
	ExpandBoundsWorld(WorldPosition);
	return static_cast<uint32>(PreviewMeshData.Vertices.size() - 1);
}

void UPhysicsAssetPreviewComponent::ExpandBoundsWorld(const FVector& WorldPosition)
{
	CachedWorldBounds.Expand(WorldPosition);
	bHasPreviewBounds = true;
}

void UPhysicsAssetPreviewComponent::AppendBox(const FTransform& ShapeWorld, const FVector& HalfExtent, const FVector4& Color)
{
	const FQuat Rotation = ShapeWorld.Rotation.GetNormalized();
	const FVector Center = ShapeWorld.Location;

	const FVector AxisX = Rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
	const FVector AxisY = Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
	const FVector AxisZ = Rotation.RotateVector(FVector(0.0f, 0.0f, 1.0f));

	auto Corner = [&](float X, float Y, float Z)
	{
		return Center + AxisX * (X * HalfExtent.X) + AxisY * (Y * HalfExtent.Y) + AxisZ * (Z * HalfExtent.Z);
	};

	auto AddFace = [&](const FVector& A, const FVector& B, const FVector& C, const FVector& D)
	{
		const uint32 Base = static_cast<uint32>(PreviewMeshData.Vertices.size());
		AddVertexWorld(A, Color);
		AddVertexWorld(B, Color);
		AddVertexWorld(C, Color);
		AddVertexWorld(D, Color);
		PreviewMeshData.Indices.push_back(Base + 0);
		PreviewMeshData.Indices.push_back(Base + 1);
		PreviewMeshData.Indices.push_back(Base + 2);
		PreviewMeshData.Indices.push_back(Base + 0);
		PreviewMeshData.Indices.push_back(Base + 2);
		PreviewMeshData.Indices.push_back(Base + 3);
	};

	const FVector P000 = Corner(-1.0f, -1.0f, -1.0f);
	const FVector P001 = Corner(-1.0f, -1.0f, 1.0f);
	const FVector P010 = Corner(-1.0f, 1.0f, -1.0f);
	const FVector P011 = Corner(-1.0f, 1.0f, 1.0f);
	const FVector P100 = Corner(1.0f, -1.0f, -1.0f);
	const FVector P101 = Corner(1.0f, -1.0f, 1.0f);
	const FVector P110 = Corner(1.0f, 1.0f, -1.0f);
	const FVector P111 = Corner(1.0f, 1.0f, 1.0f);

	AddFace(P100, P110, P111, P101);
	AddFace(P000, P001, P011, P010);
	AddFace(P010, P011, P111, P110);
	AddFace(P000, P100, P101, P001);
	AddFace(P001, P101, P111, P011);
	AddFace(P000, P010, P110, P100);
}

void UPhysicsAssetPreviewComponent::AppendSphere(const FTransform& ShapeWorld, float Radius, const FVector4& Color)
{
	const FQuat Rotation = ShapeWorld.Rotation.GetNormalized();
	const FVector Center = ShapeWorld.Location;
	const uint32 Base = static_cast<uint32>(PreviewMeshData.Vertices.size());

	for (int32 Stack = 0; Stack <= SphereStacks; ++Stack)
	{
		const float V = static_cast<float>(Stack) / static_cast<float>(SphereStacks);
		const float Phi = -0.5f * FMath::Pi + V * FMath::Pi;
		const float CP = cosf(Phi);
		const float SP = sinf(Phi);

		for (int32 Slice = 0; Slice <= SphereSlices; ++Slice)
		{
			const float U = static_cast<float>(Slice) / static_cast<float>(SphereSlices);
			const float Theta = U * 2.0f * FMath::Pi;
			const FVector LocalNormal(CP * cosf(Theta), CP * sinf(Theta), SP);
			AddVertexWorld(Center + Rotation.RotateVector(LocalNormal * Radius), Color);
		}
	}

	const uint32 RingStride = SphereSlices + 1;
	for (int32 Stack = 0; Stack < SphereStacks; ++Stack)
	{
		for (int32 Slice = 0; Slice < SphereSlices; ++Slice)
		{
			const uint32 I0 = Base + Stack * RingStride + Slice;
			const uint32 I1 = I0 + 1;
			const uint32 I2 = I0 + RingStride;
			const uint32 I3 = I2 + 1;
			PreviewMeshData.Indices.push_back(I0);
			PreviewMeshData.Indices.push_back(I2);
			PreviewMeshData.Indices.push_back(I1);
			PreviewMeshData.Indices.push_back(I1);
			PreviewMeshData.Indices.push_back(I2);
			PreviewMeshData.Indices.push_back(I3);
		}
	}
}

void UPhysicsAssetPreviewComponent::AppendCapsuleZAxis(
	const FTransform& ShapeWorld,
	float Radius,
	float HalfHeight,
	const FVector4& Color)
{
	Radius = (std::max)(Radius, MinShapeSize);
	HalfHeight = (std::max)(HalfHeight, Radius);
	const float CylinderHalf = (std::max)(0.0f, HalfHeight - Radius);

	const FQuat Rotation = ShapeWorld.Rotation.GetNormalized();
	const FVector Center = ShapeWorld.Location;
	const FVector AxisX = Rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
	const FVector AxisY = Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
	const FVector AxisZ = Rotation.RotateVector(FVector(0.0f, 0.0f, 1.0f));

	TArray<uint32> RingBases;
	auto AddRing = [&](float Z, float RingRadius)
	{
		RingBases.push_back(static_cast<uint32>(PreviewMeshData.Vertices.size()));
		for (int32 Slice = 0; Slice <= CapsuleSlices; ++Slice)
		{
			const float U = static_cast<float>(Slice) / static_cast<float>(CapsuleSlices);
			const float Theta = U * 2.0f * FMath::Pi;
			const float X = cosf(Theta) * RingRadius;
			const float Y = sinf(Theta) * RingRadius;
			AddVertexWorld(Center + AxisX * X + AxisY * Y + AxisZ * Z, Color);
		}
	};

	AddRing(-CylinderHalf - Radius, 0.0f);

	for (int32 Stack = 1; Stack <= CapsuleHemisphereStacks; ++Stack)
	{
		const float T = static_cast<float>(Stack) / static_cast<float>(CapsuleHemisphereStacks);
		const float Angle = -0.5f * FMath::Pi + T * 0.5f * FMath::Pi;
		AddRing(-CylinderHalf + sinf(Angle) * Radius, cosf(Angle) * Radius);
	}

	AddRing(CylinderHalf, Radius);

	for (int32 Stack = 1; Stack < CapsuleHemisphereStacks; ++Stack)
	{
		const float T = static_cast<float>(Stack) / static_cast<float>(CapsuleHemisphereStacks);
		const float Angle = T * 0.5f * FMath::Pi;
		AddRing(CylinderHalf + sinf(Angle) * Radius, cosf(Angle) * Radius);
	}

	AddRing(CylinderHalf + Radius, 0.0f);

	for (int32 Ring = 0; Ring + 1 < static_cast<int32>(RingBases.size()); ++Ring)
	{
		const uint32 R0 = RingBases[Ring];
		const uint32 R1 = RingBases[Ring + 1];
		for (int32 Slice = 0; Slice < CapsuleSlices; ++Slice)
		{
			const uint32 I0 = R0 + Slice;
			const uint32 I1 = R0 + Slice + 1;
			const uint32 I2 = R1 + Slice;
			const uint32 I3 = R1 + Slice + 1;
			PreviewMeshData.Indices.push_back(I0);
			PreviewMeshData.Indices.push_back(I2);
			PreviewMeshData.Indices.push_back(I1);
			PreviewMeshData.Indices.push_back(I1);
			PreviewMeshData.Indices.push_back(I2);
			PreviewMeshData.Indices.push_back(I3);
		}
	}
}
