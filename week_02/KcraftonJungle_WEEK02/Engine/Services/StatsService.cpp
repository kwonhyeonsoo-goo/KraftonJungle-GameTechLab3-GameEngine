#include "StatsService.h"
#include "Engine/Foundation/Core/Memory.h"

EngineStats StatsService::Collect(const AppContext& ctx) {
    EngineStats stats;
    stats.ObjectCount = ctx.Objects.Count();
    stats.TotalAllocBytes = EngineMemory::TotalAllocationBytes;
    stats.TotalAllocCount = EngineMemory::TotalAllocationCount;
    stats.SelectedObjectCount = ctx.Editor.Selection.Count();
    return stats;
}
