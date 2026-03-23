#include "ObjectGlobals.h"

ENGINE_API FMalloc GMalloc;

void* operator new(size_t Size)
{
	return GMalloc.Malloc(Size);
}

void operator delete(void* Ptr) noexcept
{
	GMalloc.Free(Ptr);
}

void* operator new[](size_t Size)
{
	return GMalloc.Malloc(Size);
}

void operator delete[](void* Ptr) noexcept
{
	GMalloc.Free(Ptr);
}