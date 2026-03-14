#include "Memory.h"

namespace EngineMemory
{
    uint32 TotalAllocationBytes = 0;
    uint32 TotalAllocationCount = 0;

    void* Allocate(uint32 size)
    {
        const uint32 TotalSize = size + sizeof(uint32);

        void* raw = std::malloc(TotalSize);
        if (!raw)
            throw std::bad_alloc();

        std::memcpy(raw, &size, sizeof(uint32));

        TotalAllocationBytes += size;
        TotalAllocationCount += 1;

        return static_cast<char*>(raw) + sizeof(uint32);
    }

    void Free(void* ptr)
    {
        if (ptr == nullptr)
            return;

        void* raw = static_cast<char*>(ptr) - sizeof(uint32);

        uint32 size = 0;
        std::memcpy(&size, raw, sizeof(uint32));

        if (TotalAllocationBytes >= size)
            TotalAllocationBytes -= size;
        else
            TotalAllocationBytes = 0;

        if (TotalAllocationCount > 0)
            TotalAllocationCount -= 1;

        std::free(raw);
    }
}

void* operator new(std::size_t size)
{
    return EngineMemory::Allocate(static_cast<uint32>(size));
}

void operator delete(void* ptr) noexcept
{
    EngineMemory::Free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept
{
    EngineMemory::Free(ptr);
}

void* operator new[](std::size_t size)
{
    return EngineMemory::Allocate(static_cast<uint32>(size));
}

void operator delete[](void* ptr) noexcept
{
    EngineMemory::Free(ptr);
}

void operator delete[](void* ptr, std::size_t /*size*/) noexcept
{
    EngineMemory::Free(ptr);
}