#pragma once
#include "CoreTypes.h"
#include "Log.h"

#include <cstdlib>
#include <cstring>
#include <new>

namespace EngineMemory
{
    extern uint32 TotalAllocationBytes;
    extern uint32 TotalAllocationCount;

    void* Allocate(uint32 size);
    void  Free(void* ptr);

    inline void PrintStats()
    {
        UE_LOG("[Memory] Alloc: %u bytes, Count: %u",
            TotalAllocationBytes, TotalAllocationCount);
    }
}

void* operator new(std::size_t size);
void  operator delete(void* ptr) noexcept;
void  operator delete(void* ptr, std::size_t size) noexcept;

void* operator new[](std::size_t size);
void  operator delete[](void* ptr) noexcept;
void  operator delete[](void* ptr, std::size_t size) noexcept;