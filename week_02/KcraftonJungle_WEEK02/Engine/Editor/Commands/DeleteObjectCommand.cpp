#include "DeleteObjectCommand.h"

DeleteObjectCommand::DeleteObjectCommand(AppContext& ctx, uint32 uuid) : Ctx(ctx), TargetUUID(uuid)
{
}

void DeleteObjectCommand::Execute()
{
	if (TargetUUID == 0) return;
	UObject* Obj = Ctx.Objects.Find(TargetUUID);
	if (Obj == nullptr || !Obj->IsA<UPrimitiveComponent>()) return;
	UPrimitiveComponent* Target = static_cast<UPrimitiveComponent*>(Obj);

	SavedShape = Target->Shape;
	SavedTransform = Target->GetTransform();

	Ctx.Editor.Selection.OnObjectDestroyed({ TargetUUID });
	Ctx.OnObjectDestroyed.Broadcast({ TargetUUID });
	Ctx.Objects.Remove(TargetUUID);
}

void DeleteObjectCommand::Undo()
{
	if (TargetUUID == 0) return;
}

FString DeleteObjectCommand::GetDescription() const
{
	return "Delete Object";
}

