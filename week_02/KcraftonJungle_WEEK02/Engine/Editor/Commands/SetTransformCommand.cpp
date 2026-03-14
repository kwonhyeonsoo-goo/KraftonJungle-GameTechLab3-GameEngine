#include "SetTransformCommand.h"

SetTransformCommand::SetTransformCommand(USceneComponent* target, const Transform& newTransform)
{
	if (target == nullptr) return;
	Target = target;
	OldTransform = Target->GetTransform();
	NewTransform = newTransform;
}

void SetTransformCommand::Execute()
{
	if (Target != nullptr) Target->SetTransform(NewTransform);
}

void SetTransformCommand::Undo()
{
	if (Target != nullptr) Target->SetTransform(OldTransform);
}

FString SetTransformCommand::GetDescription() const
{
	return "Set Transform";
}

