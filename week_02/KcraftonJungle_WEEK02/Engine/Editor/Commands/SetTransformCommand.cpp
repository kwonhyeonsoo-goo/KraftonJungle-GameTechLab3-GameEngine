#include "SetTransformCommand.h"

SetTransformCommand::SetTransformCommand(USceneComponent* target, const Transform& newTransform)
    : Target(target), OldTransform(), NewTransform(newTransform)
{
    if (Target != nullptr)
    {
        OldTransform = Target->GetTransform();
    }
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

