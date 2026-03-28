#pragma once
#include "Types/CoreTypes.h"
#include "Types/Array.h"

class UPrimitiveComponent;
class UStaticMeshComponent;

class FScene
{
public:
	void AddComponent(UPrimitiveComponent* Comp);

	void Clear();

	/** 다소 비효율적으로 반환하는 측면이 있어 자주 호출된다면 해결이 필요할 수 있음 */
	const TArray<UStaticMeshComponent*> GetStaticMeshComponents() const;

private:
	/** Proxy 개념은 따로 적용하지 않음. Actor Destroy -> Primitive Delete 적용 필요 */
	TArray<UPrimitiveComponent*> Primitives;
};
