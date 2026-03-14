#pragma once
#include "../Foundation/Core/CoreTypes.h"

class UUIDService {
public:
    UUIDService();

    uint32 GenUUID();

    void   SyncNextUUID(uint32 value);
    uint32 GetNext() const;

private:
    uint32 NextUUID = 1;
};