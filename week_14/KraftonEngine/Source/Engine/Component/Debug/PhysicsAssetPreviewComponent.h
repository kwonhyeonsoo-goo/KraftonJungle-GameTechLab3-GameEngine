#pragma once

#include "Component/PrimitiveComponent.h"
#include "Object/Ptr/WeakObjectPtr.h"
#include "Physics/PhysicsAssetTypes.h"
#include "Render/Resource/Buffer.h"
#include "Render/Types/VertexTypes.h"

#include "Source/Engine/Component/Debug/PhysicsAssetPreviewComponent.generated.h"

class UPhysicsAsset;
class USkeletalMeshComponent;
struct ID3D11Device;

UCLASS()
class UPhysicsAssetPreviewComponent : public UPrimitiveComponent
{
public:
	GENERATED_BODY()

	UPhysicsAssetPreviewComponent();
	~UPhysicsAssetPreviewComponent() override;

	FPrimitiveSceneProxy* CreateSceneProxy() override;
	FMeshBuffer* GetMeshBuffer() const override;
	FMeshDataView GetMeshDataView() const override;
	void UpdateWorldAABB() const override;
	bool SupportsOutline() const override { return false; }

	void UpdatePreview(
		UPhysicsAsset* InPhysicsAsset,
		USkeletalMeshComponent* InPreviewComponent,
		int32 InSelectedBodyIndex,
		int32 InSelectedShapeIndex,
		int32 InSelectedConstraintIndex,
		bool bInShowBodies,
		ID3D11Device* Device);

	void ClearPreview(ID3D11Device* Device = nullptr);

private:
	void RebuildPreviewMesh();
	void UploadPreviewMesh(ID3D11Device* Device);

	void AppendBox(const FTransform& ShapeWorld, const FVector& HalfExtent, const FVector4& Color);
	void AppendSphere(const FTransform& ShapeWorld, float Radius, const FVector4& Color);
	void AppendCapsuleZAxis(const FTransform& ShapeWorld, float Radius, float HalfHeight, const FVector4& Color);

	uint32 AddVertexWorld(const FVector& WorldPosition, const FVector4& Color);
	void ExpandBoundsWorld(const FVector& WorldPosition);

private:
	TWeakObjectPtr<UPhysicsAsset> PhysicsAsset;
	TWeakObjectPtr<USkeletalMeshComponent> PreviewComponent;

	int32 SelectedBodyIndex = -1;
	int32 SelectedShapeIndex = -1;
	int32 SelectedConstraintIndex = -1;
	bool bShowBodies = true;

	TMeshData<FVertex> PreviewMeshData;
	FMeshBuffer PreviewMeshBuffer;

	FBoundingBox CachedWorldBounds;
	bool bHasPreviewBounds = false;
};
