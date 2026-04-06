#include "SceneComponent.h"
#include <algorithm>

FSceneComponent::FSceneComponent() = default;

FSceneComponent::~FSceneComponent()
{
	DetachFromParent();

	for (FSceneComponent* Child : Children)
	{
		if (Child)
		{
			Child->Parent = nullptr;
		}
	}
}

void FSceneComponent::Tick()
{
	if (bIsWorldTransformDirty)
	{
		GetComponentToWorld();
	}
}

void FSceneComponent::AttachTo(FSceneComponent* InParent)
{
	if (Parent == InParent) return;

	DetachFromParent();

	Parent = InParent;
	if (Parent)
	{
		Parent->Children.push_back(this);
	}
	MarkRenderStateDirty();
}

void FSceneComponent::DetachFromParent()
{
	if (Parent)
	{
		auto It = std::find(Parent->Children.begin(), Parent->Children.end(), this);
		if (It != Parent->Children.end())
		{
			Parent->Children.erase(It);
		}
		Parent = nullptr;
		MarkRenderStateDirty();
	}
}

void FSceneComponent::SetRelativeLocation(const FVector& InLocation)
{
	RelativeLocation = InLocation;
	MarkRenderStateDirty();
}

void FSceneComponent::SetRelativeRotation(const FQuat& InRotation)
{
	RelativeRotation = InRotation;
	MarkRenderStateDirty();
}

void FSceneComponent::SetRelativeScale(const FVector& InScale)
{
	RelativeScale = InScale;
	MarkRenderStateDirty();
}

void FSceneComponent::SetWorldMatrix(const FMatrix& InWorldMatrix)
{

	FMatrix LocalMatrix = InWorldMatrix;
	if (Parent)
	{
		LocalMatrix = InWorldMatrix * Parent->GetComponentToWorld().GetInverse();
	}

	RelativeLocation = LocalMatrix.GetOrigin();
	RelativeScale = LocalMatrix.ExtractScale();
	RelativeRotation = FQuat(LocalMatrix); // 행렬을 Quat으로 변환

	// 3. 변경되었으니 자신과 자식들의 깃발을 켬
	MarkRenderStateDirty();
}

FVector FSceneComponent::GetWorldLocation() const
{
	return GetComponentToWorld().GetOrigin();
}

FQuat FSceneComponent::GetWorldRotation() const
{
	if (Parent)
	{
		return Parent->GetWorldRotation() * RelativeRotation;
	}
	return RelativeRotation;
}

FVector FSceneComponent::GetWorldScale() const
{
	if (Parent)
	{
		return Parent->GetWorldScale() * RelativeScale;
	}
	return RelativeScale;
}

void FSceneComponent::MarkRenderStateDirty()
{
	// [핵심] 이미 깃발이 켜져 있으면, 내 자식들도 이미 다 켜져있다는 뜻이므로 즉시 리턴 (O(1) 조기 종료)
	if (bIsWorldTransformDirty) return;

	bIsWorldTransformDirty = true;

	for (FSceneComponent* Child : Children)
	{
		if (Child)
		{
			Child->MarkRenderStateDirty();
		}
	}
}

const FMatrix& FSceneComponent::GetComponentToWorld() const
{
	// [핵심] 누군가 렌더링 등을 위해 행렬을 요구할 때, 깃발이 켜져 있을 때만 1번 재계산.
	// 움직이지 않았다면 예전 행렬을 그대로 반환 (연산량 0)
	if (bIsWorldTransformDirty)
	{
		bIsWorldTransformDirty = false;
		UpdateComponentToWorld();
	}
	return ComponentToWorld;
}

void FSceneComponent::UpdateComponentToWorld() const
{
	// Scale * Rotation * Translation 순서로 로컬 행렬 생성 (DirectX Row-Major 기준)
	FMatrix LocalMatrix = FMatrix::MakeScale(RelativeScale) *
		FMatrix::MakeRotation(RelativeRotation) *
		FMatrix::MakeTranslation(RelativeLocation);

	if (Parent)
	{
		// 로컬 행렬 * 부모의 월드 행렬 = 나의 최종 월드 행렬
		ComponentToWorld = LocalMatrix * Parent->GetComponentToWorld();
	}
	else
	{
		ComponentToWorld = LocalMatrix;
	}

	// 트랜스폼 갱신 직후, 이를 상속받은 메쉬 컴포넌트 등이 바운딩 박스(AABB)를 업데이트할 수 있도록 호출
	const_cast<FSceneComponent*>(this)->OnUpdateWorldTransform();
}