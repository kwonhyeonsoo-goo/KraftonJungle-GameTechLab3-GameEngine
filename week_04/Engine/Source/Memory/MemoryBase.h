#pragma once
#include "CoreMinimal.h"
#include <cstddef>

struct FAllocHeader
{
	uint32 MagicNumber;
	SIZE_T Size;
	/** 동적 데이터는 문제 발생 가능 (size 를 미리 알 수 없음) */
};

constexpr uint32 MALLOC_MAGIC = 0xDEADBEEF;
// Header 뒤 포인터가 최소 MAX_ALIGN 정렬을 만족하도록 패딩 포함한 크기
constexpr SIZE_T HEADER_STRIDE =
(sizeof(FAllocHeader) + alignof(std::max_align_t) - 1) & ~(alignof(std::max_align_t) - 1);

struct FMallocStats
{
	uint32 CurrentAllocationBytes = 0;
	uint32 TotalAllocationBytes = 0;

	uint32 CurrentAllocationCount = 0;
	uint32 TotalAllocationCount = 0;
};

class FMalloc
{
public:
	virtual void* Malloc(SIZE_T Count);
	virtual void  Free(void* Original);

	FMallocStats MallocStats;
};
