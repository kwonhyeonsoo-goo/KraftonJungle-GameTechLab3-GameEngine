#pragma once

#include "GizmoTransformTarget.h"
#include "Math/Transform.h"

class USkeletalMeshComponent;
class USkeleton;

class FBoneTransformGizmoTarget : public IGizmoTransformTarget
{
public:
	FBoneTransformGizmoTarget();
	FBoneTransformGizmoTarget(USkeletalMeshComponent* InMeshComp, int32 InBoneIndex);
	~FBoneTransformGizmoTarget() override = default;

public:
	void SetBone(USkeletalMeshComponent* MeshComp, int32 BoneIndex) { MeshComponent = MeshComp; this->BoneIndex = BoneIndex; }

	bool IsValid() const override;
	UWorld* GetWorld() const override;

	FVector GetWorldLocation() const override;
	FRotator GetWorldRotation() const override;
	FQuat GetWorldQuat() const override;
	FVector GetWorldScale() const override;

	void SetWorldLocation(const FVector& NewLocation) override;
	void SetWorldRotation(const FRotator& NewRotation) override;
	void SetWorldRotation(const FQuat& NewQuat) override;
	void SetWorldScale(const FVector& NewScale) override;

	void AddWorldOffset(const FVector& Delta) override;
	void AddWorldRotation(const FQuat& Delta, bool bWorldSpace) override;
	void AddScaleDelta(const FVector& Delta) override;

private:
	USkeletalMeshComponent* MeshComponent = nullptr;
	int32 BoneIndex = -1;
};

class FSocketTransformGizmoTarget : public IGizmoTransformTarget
{
public:
	FSocketTransformGizmoTarget();
	FSocketTransformGizmoTarget(USkeletalMeshComponent* InMeshComp, USkeleton* InSkeleton, int32 InSocketIndex);
	~FSocketTransformGizmoTarget() override = default;

	void SetSocket(USkeletalMeshComponent* MeshComp, USkeleton* InSkeleton, int32 InSocketIndex);
	bool ConsumeModified();

	bool IsValid() const override;
	UWorld* GetWorld() const override;

	FVector GetWorldLocation() const override;
	FRotator GetWorldRotation() const override;
	FQuat GetWorldQuat() const override;
	FVector GetWorldScale() const override;

	void SetWorldLocation(const FVector& NewLocation) override;
	void SetWorldRotation(const FRotator& NewRotation) override;
	void SetWorldRotation(const FQuat& NewQuat) override;
	void SetWorldScale(const FVector& NewScale) override;

	void AddWorldOffset(const FVector& Delta) override;
	void AddWorldRotation(const FQuat& Delta, bool bWorldSpace) override;
	void AddScaleDelta(const FVector& Delta) override;

private:
	bool GetSocketWorldTransform(FTransform& OutTransform) const;
	bool ApplySocketWorldTransform(const FTransform& NewWorldTransform);

private:
	USkeletalMeshComponent* MeshComponent = nullptr;
	USkeleton* Skeleton = nullptr;
	int32 SocketIndex = -1;
	bool bModified = false;
};
