#include "Memory.h"

void* EngineMemory::Allocate(uint32 size)
{
    uint32 TotalSize = size + sizeof(uint32);
    void* raw = malloc(TotalSize);
    if (!raw) throw std::bad_alloc();;
    memcpy(raw, &size, sizeof(uint32));
    TotalAllocationBytes += size;
    TotalAllocationCount++;

    return static_cast<char*>(raw) + sizeof(uint32);
}

void EngineMemory::Free(void* ptr)
{
    if (ptr == nullptr) return;
    void* raw = static_cast<char*>(ptr) - sizeof(uint32);
    uint32 size = 0;
    memcpy(&size, raw, sizeof(uint32));
    TotalAllocationBytes -= size;
    if (TotalAllocationCount > 0) TotalAllocationCount--;
    free(raw);
}

void* operator new(std::size_t size)
{
    return EngineMemory::Allocate(size);
}

void operator delete(void* ptr) noexcept
{
    EngineMemory::Free(ptr);
}
