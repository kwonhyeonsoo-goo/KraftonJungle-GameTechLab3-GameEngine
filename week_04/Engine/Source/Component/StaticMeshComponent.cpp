#include "StaticMeshComponent.h"
#include "Object/Class.h"

IMPLEMENT_RTTI(UStaticMeshComponent, UMeshComponent)

void UStaticMeshComponent::SetStaticMesh(UStaticMesh* InMesh)
{
	OverrideMaterials.clear();
	StaticMesh = InMesh;

	/** InMesh 존재 시 OverrideMaterials 크기 조정 및 값 복사 (FStaticMesh 코드 대기중) */
}
