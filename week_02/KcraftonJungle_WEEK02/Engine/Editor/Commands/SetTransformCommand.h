#pragma once
#include "ICommand.h"
#include "../../World/USceneComponent.h"

class SetTransformCommand : public ICommand {
public:
    SetTransformCommand(USceneComponent* target = nullptr, const Transform& newTransform = Transform());

    void Execute() override;
    void Undo()    override;
    FString GetDescription() const override;

private:
    USceneComponent* Target;
    Transform OldTransform;
    Transform NewTransform;
};