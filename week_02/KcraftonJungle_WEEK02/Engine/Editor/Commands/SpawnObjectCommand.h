#pragma once
#include "../../../AppContext.h"
#include "../../World/UPrimitiveComponent.h"
#include "../../World/Transform.h"

class SpawnObjectCommand : public ICommand {
public:
    SpawnObjectCommand(AppContext& ctx, EPrimitiveShape shape, const Transform& spawnTransform);

    void Execute() override;
    void Undo() override;
    FString GetDescription() const override;

    uint32 GetSpawnedUUID() const;

private:
    AppContext& Ctx;
    EPrimitiveShape Shape;
    Transform SpawnTransform;
    uint32 SpawnedUUID = 0;
};