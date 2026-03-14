#pragma once
#include <cstdint>

struct EngineStats {
    uint32_t ObjectCount;
    uint32_t TotalAllocBytes;
    uint32_t TotalAllocCount;
    uint32_t SelectedObjectCount;
};

struct AppContext;

class StatsService
{
public:
    static EngineStats Collect(const AppContext& ctx);
};

