#include "ActorComponent.h"
#include "Object/Class.h"
#include "Serializer/Archive.h"
IMPLEMENT_RTTI(UActorComponent, UObject)

void UActorComponent::Serialize(FArchive& Ar)
{
	UObject::Serialize(Ar);

	// 2. 모든 컴포넌트가 공통으로 가지는 기본 속성 직렬화
	if (Ar.IsSaving())
	{
		Ar.Serialize("bTickEnabled", bTickEnabled);
	}
	else // IsLoading
	{
		if (Ar.Contains("bTickEnabled"))
		{
			Ar.Serialize("bTickEnabled", bTickEnabled);
		}
	}
}
