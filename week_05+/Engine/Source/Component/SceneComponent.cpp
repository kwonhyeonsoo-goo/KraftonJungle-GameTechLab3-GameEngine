#include "SceneComponent.h"
#include "Object/Class.h"
#include "Serializer/Archive.h"
IMPLEMENT_RTTI(USceneComponent, UActorComponent)

void USceneComponent::SetRelativeTransform(const FTransform& InTransform)
{
	RelativeTransform = InTransform;
	MarkTransformDirty();
}

void USceneComponent::SetRelativeLocation(const FVector& InLocation)
{
	RelativeTransform.SetTranslation(InLocation);
	MarkTransformDirty();
}

void USceneComponent::AttachTo(USceneComponent* InParent)
{
	if (AttachParent == InParent)
	{
		return;
	}

	DetachFromParent();

	AttachParent = InParent;
	if (AttachParent)
	{
		AttachParent->AttachChildren.push_back(this);
	}

	MarkTransformDirty();
}

void USceneComponent::DetachFromParent()
{
	if (AttachParent == nullptr)
	{
		return;
	}

	auto& Siblings = AttachParent->AttachChildren;
	std::erase(Siblings, this);
	AttachParent = nullptr;

	MarkTransformDirty();
}

const FMatrix& USceneComponent::GetWorldTransform() const
{
	if (bWorldTransformDirty)
	{
		UpdateWorldTransform();
	}
	return CachedWorldTransform;
}

void USceneComponent::Serialize(FArchive& Ar)
{
	UActorComponent::Serialize(Ar);

	// 2. USceneComponent의 핵심: Transform(위치, 회전, 크기) 직렬화
	if (Ar.IsSaving())
	{
		FVector Location = RelativeTransform.GetTranslation();
		FVector Rotation = RelativeTransform.Rotator().Euler();
		FVector Scale = RelativeTransform.GetScale3D();

		Ar.Serialize("Location", Location);
		Ar.Serialize("Rotation", Rotation);
		Ar.Serialize("Scale", Scale);
	}
	else // IsLoading()
	{
		FVector Location = RelativeTransform.GetTranslation();
		FVector Rotation = RelativeTransform.Rotator().Euler();
		FVector Scale = RelativeTransform.GetScale3D();

		if (Ar.Contains("Location"))
		{
			Ar.Serialize("Location", Location);
			RelativeTransform.SetTranslation(Location);
		}

		if (Ar.Contains("Rotation"))
		{
			Ar.Serialize("Rotation", Rotation);
			RelativeTransform.SetRotation(FRotator::MakeFromEuler(Rotation));
		}

		if (Ar.Contains("Scale"))
		{
			Ar.Serialize("Scale", Scale);
			RelativeTransform.SetScale3D(Scale);
		}

		// ⭐ [핵심 포인트] 로드가 끝난 후 트랜스폼이 변경되었으므로 Dirty 플래그를 켜줍니다.
		// 이렇게 하면 이 컴포넌트에 붙어있는 자식 컴포넌트들까지 연쇄적으로 위치가 갱신됩니다!
		MarkTransformDirty();
	}
}

FVector USceneComponent::GetWorldLocation() const
{
	return GetWorldTransform().GetTranslation();
}

void USceneComponent::MarkTransformDirty()
{
	bWorldTransformDirty = true;

	for (USceneComponent* Child : AttachChildren)
	{
		if (Child)
		{
			Child->MarkTransformDirty();
		}
	}
}

void USceneComponent::UpdateWorldTransform() const
{
	CachedWorldTransform = RelativeTransform.ToMatrixWithScale();
	if (AttachParent)
	{
		CachedWorldTransform = CachedWorldTransform * AttachParent->GetWorldTransform();
	}
	bWorldTransformDirty = false;
}
