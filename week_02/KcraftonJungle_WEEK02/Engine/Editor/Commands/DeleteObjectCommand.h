#pragma once
#include "ICommand.h"
#include "../../World/UPrimitiveComponent.h"
#include "../../../AppContext.h"

class DeleteObjectCommand : public ICommand {
public:
    explicit DeleteObjectCommand(AppContext& ctx, uint32 uuid);

    void Execute() override;
    void Undo() override;
    FString GetDescription() const override;

private:
    AppContext& Ctx;
    uint32 TargetUUID = 0;
    EPrimitiveShape SavedShape;
    Transform SavedTransform;
};