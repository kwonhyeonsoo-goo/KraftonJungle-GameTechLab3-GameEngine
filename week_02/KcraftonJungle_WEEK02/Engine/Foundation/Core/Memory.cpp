#include "Memory.h"
#include <cstdint>
#include <limits>

namespace EngineMemory
{
    struct Header
    {
        std::size_t RequestedSize; // 사용자가 요청한 크기
        void* RawBlock;            // std::malloc이 반환한 원래 주소
        std::uint32_t Magic;       // 유효성 체크용
    };

    static const std::uint32_t kMagic = 0xC0FFEE11u;

    std::atomic<std::size_t> EngineMemory::TotalAllocationBytes{ 0 };
    std::atomic<std::size_t> EngineMemory::TotalAllocationCount{ 0 };

    inline std::uintptr_t AlignUp(std::uintptr_t value, std::size_t alignment) noexcept
    {
        const std::uintptr_t remainder = value % alignment;
        if (remainder == 0)
            return value;

        return value + (alignment - remainder);
    }

    inline static void* AllocateRawWithNewHandler(std::size_t totalSize)
    {
        for (;;)
        {
            if (void* p = std::malloc(totalSize))
                return p;

            std::new_handler handler = std::get_new_handler();
            if (!handler)
                throw std::bad_alloc();

            handler();
        }
    }

    void* Allocate(std::size_t size)
    {
        const std::size_t actualUserSize = (size == 0) ? 1 : size;

        const std::size_t alignment = alignof(std::max_align_t);
        const std::size_t extra = sizeof(Header) + (alignment - 1);

        if (actualUserSize > (std::numeric_limits<std::size_t>::max)() - extra)
            throw std::bad_alloc();

        const std::size_t totalSize = actualUserSize + extra;

        void* raw = AllocateRawWithNewHandler(totalSize);

        const std::uintptr_t rawAddr = reinterpret_cast<std::uintptr_t>(raw);
        const std::uintptr_t userAddr = AlignUp(rawAddr + sizeof(Header), alignment);

        Header* header = reinterpret_cast<Header*>(userAddr - sizeof(Header));
        header->RequestedSize = size;
        header->RawBlock = raw;
        header->Magic = kMagic;

        TotalAllocationBytes.fetch_add(size, std::memory_order_relaxed);
        TotalAllocationCount.fetch_add(1, std::memory_order_relaxed);
        return reinterpret_cast<void*>(userAddr);
    }

    void Free(void* ptr) noexcept
    {
        if (ptr == nullptr)
            return;

        Header* header = reinterpret_cast<Header*>(
            reinterpret_cast<std::uintptr_t>(ptr) - sizeof(Header));

        if (header->Magic != kMagic)
        {
            std::abort();
        }

        header->Magic = 0;

        TotalAllocationBytes.fetch_sub(header->RequestedSize, std::memory_order_relaxed);
        TotalAllocationCount.fetch_sub(1, std::memory_order_relaxed);

        std::free(header->RawBlock);
    }
}

void* operator new(std::size_t size)
{
    return EngineMemory::Allocate(size);
}

void operator delete(void* ptr) noexcept
{
    EngineMemory::Free(ptr);
}

void* operator new[](std::size_t size)
{
    return EngineMemory::Allocate(size);
}

void operator delete[](void* ptr) noexcept
{
    EngineMemory::Free(ptr);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    try
    {
        return EngineMemory::Allocate(size);
    }
    catch (...)
    {
        return nullptr;
    }
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept
{
    EngineMemory::Free(ptr);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
    try
    {
        return EngineMemory::Allocate(size);
    }
    catch (...)
    {
        return nullptr;
    }
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
    EngineMemory::Free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept
{
    EngineMemory::Free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept
{
    EngineMemory::Free(ptr);
}