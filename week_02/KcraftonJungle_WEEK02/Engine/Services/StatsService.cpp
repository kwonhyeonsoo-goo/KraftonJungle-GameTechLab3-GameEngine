#include "StatsService.h"
#include "../Foundation/Core/Memory.h"
#include "../../AppContext.h"

EngineStats StatsService::Collect(const AppContext& ctx) {
    EngineStats stats;
    stats.ObjectCount = ctx.Objects.Count();
    stats.TotalAllocBytes = EngineMemory::TotalAllocationBytes;
    stats.TotalAllocCount = EngineMemory::TotalAllocationCount;
    stats.SelectedObjectCount = ctx.Editor.Selection.Count();
    return stats;
}
