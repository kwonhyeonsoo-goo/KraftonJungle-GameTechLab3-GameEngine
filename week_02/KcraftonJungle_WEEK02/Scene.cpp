#include "Scene.h"

UScene::~UScene()
{
}

void UScene::Add(UPrimitiveComponent* comp)
{
    /*Ctx.Objects.Add(comp);
    Ctx->UUIDsUUIDs.Add(comp->GetUUID());*/
}

void UScene::Remove(uint32 uuid)
{
    /*Ctx->Objects.Remove(uuid);
    Ctx->UUIDs.Add(comp->GetUUID());*/
}

UPrimitiveComponent* UScene::Find(uint32 uuid) const
{
    //오브 젝트 스토어에서 찾거나 데이터 갖고 오기
    //Ctx->Objects.Find(uint32 uuid);
    return nullptr;
}

const TArray<UObject*>& UScene::GetAllObjects() const
{
    // TODO: 여기에 return 문을 삽입합니다.
    /*return Ctx.GUObjectArray() */
}

