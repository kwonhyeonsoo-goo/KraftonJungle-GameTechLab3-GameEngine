#include "Scene.h"
#include "Component/PrimitiveComponent.h"
#include "Component/StaticMeshComponent.h"

void FScene::AddPrimitive(FMeshData* InPrimitive)
{
	Primitives.push_back(InPrimitive);
}

void FScene::RemovePrimitive(FMeshData* InPrimitive)
{
	/** 삭제 시 기존 iterator 가 무효화되는 것 인지 (e.g., 반복문 안 삭제) */
	std::erase(Primitives, InPrimitive);
}
