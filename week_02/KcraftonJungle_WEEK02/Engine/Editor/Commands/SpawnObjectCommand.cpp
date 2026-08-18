#include "SpawnObjectCommand.h"

SpawnObjectCommand::SpawnObjectCommand(AppContext& ctx, EPrimitiveShape shape, const Transform& spawnTransform) : Ctx(ctx), Shape(shape), SpawnTransform(spawnTransform)
{
}

void SpawnObjectCommand::Execute()
{
	USceneComponent* SpawnedObject = static_cast<USceneComponent*>(Ctx.Factory.ConstructObject(Ctx, PrimitiveShapeToString(Shape)));
	if (SpawnedObject == nullptr) return;
	SpawnedObject->SetTransform(SpawnTransform);
	SpawnedUUID = SpawnedObject->GetUUID();

}

void SpawnObjectCommand::Undo()
{
	if (SpawnedUUID == 0) return;
	Ctx.Editor.Selection.OnObjectDestroyed({ SpawnedUUID });
	Ctx.OnObjectDestroyed.Broadcast({ SpawnedUUID });
	Ctx.Objects.Remove(SpawnedUUID);
	SpawnedUUID = 0;
}

FString SpawnObjectCommand::GetDescription() const
{
	return "Spawn Object";
}

uint32 SpawnObjectCommand::GetSpawnedUUID() const
{
	return SpawnedUUID;
}