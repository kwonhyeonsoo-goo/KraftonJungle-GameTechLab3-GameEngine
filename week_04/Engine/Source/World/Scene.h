#pragma once
#include "Types/CoreTypes.h"
#include "Types/Array.h"

class UPrimitiveComponent;
class UStaticMeshComponent;
struct FMeshData;

/** 당장은 FMeshData -> FPrimitiveSceneProxy 개념으로 생각하면 됨 **/
class FScene
{
public:
	// Primitive Register / Unregister 시 추가/삭제 진행
	void AddPrimitive(FMeshData* InPrimitive);
	void RemovePrimitive(FMeshData* InPrimitive);

	uint32 Size() const { return Primitives.size(); }
private:
	TArray<FMeshData*> Primitives;
};
