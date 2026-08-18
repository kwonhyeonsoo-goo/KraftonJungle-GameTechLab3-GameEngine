#pragma once

#include "CoreTypes.h"
#include "Log.h"
#include <iostream>
#include <atomic>
#include <cstddef>
#include <new>

namespace EngineMemory
{
    extern std::atomic<std::size_t> TotalAllocationBytes;
    extern std::atomic<std::size_t> TotalAllocationCount;

    void* Allocate(std::size_t size);
    void  Free(void* ptr) noexcept;

    inline std::size_t GetTotalAllocationBytes() noexcept
    {
        return TotalAllocationBytes.load(std::memory_order_relaxed);
    }

    inline std::size_t GetTotalAllocationCount() noexcept
    {
        return TotalAllocationCount.load(std::memory_order_relaxed);
    }

    inline void PrintStats()
    {
        UE_LOG("[Memory] Alloc: %zu bytes, Count: %zu",
            GetTotalAllocationBytes(),
            GetTotalAllocationCount());
    }
}

void* operator new(std::size_t size);
void  operator delete(void* ptr) noexcept;
void  operator delete(void* ptr, std::size_t size) noexcept;

void* operator new[](std::size_t size);
void  operator delete[](void* ptr) noexcept;
void  operator delete[](void* ptr, std::size_t size) noexcept;

void* operator new(std::size_t size, const std::nothrow_t&) noexcept;
void  operator delete(void* ptr, const std::nothrow_t&) noexcept;

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept;
void  operator delete[](void* ptr, const std::nothrow_t&) noexcept;