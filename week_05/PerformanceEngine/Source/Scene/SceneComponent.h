#pragma once

#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "Math/Quat.h"
#include "Types/Array.h"

class FSceneComponent
{
public:
	FSceneComponent();
	virtual ~FSceneComponent();

	void Tick();

	// 계층 구조 (Hierarchy)
	void AttachTo(FSceneComponent* InParent);
	void DetachFromParent();
	FSceneComponent* GetParent() const { return Parent; }
	const TArray<FSceneComponent*>& GetChildren() const { return Children; }

	// 로컬 트랜스폼
	void SetRelativeLocation(const FVector& InLocation);
	void SetRelativeRotation(const FQuat& InRotation);
	void SetRelativeScale(const FVector& InScale);
	void SetWorldMatrix(const FMatrix& InWorldMatrix);
	const FVector& GetRelativeLocation() const { return RelativeLocation; }
	const FQuat& GetRelativeRotation() const { return RelativeRotation; }
	const FVector& GetRelativeScale() const { return RelativeScale; }

	// 월드 트랜스폼 정보 조회
	FVector GetWorldLocation() const;
	FQuat GetWorldRotation() const;
	FVector GetWorldScale() const;

	// 월드 행렬 반환 (필요할 때만 재계산됨)
	const FMatrix& GetComponentToWorld() const;

	// 자신과 자식들의 행렬 재계산 예약 (Dirty Flag 켜기)
	void MarkRenderStateDirty();
protected:
	// 상속받은 클래스(MeshComponent 등)가 트랜스폼 변경 시 추가 작업을 할 수 있도록 지원
	virtual void OnUpdateWorldTransform() {}

private:
	void UpdateComponentToWorld() const;

private:
	FSceneComponent* Parent = nullptr;
	TArray<FSceneComponent*> Children;

	FVector RelativeLocation = FVector::ZeroVector;
	FQuat RelativeRotation = FQuat::Identity;
	FVector RelativeScale = FVector::OneVector;

	// 캐싱된 월드 행렬 (Dirty Flag Pattern)
	mutable FMatrix ComponentToWorld = FMatrix::Identity;
	mutable bool bIsWorldTransformDirty = true; // 초기엔 무조건 한 번 계산하도록 true 설정
};