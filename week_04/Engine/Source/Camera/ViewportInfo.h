#pragma once
#include "../CoreMinimal.h"

struct FViewportInfo
{

	float TopLeftX = 0.f;
	float TopLeftY = 0.f;
	float Width = 0.f;
	float Height = 0.f;
	float MinDepth = 0.f;
	float MaxDepth = 1.f;

	// 피킹용 — 윈도우 마우스 좌표 → 뷰포트 로컬 좌표 변환
	int32 ClientPosX = 0;
	int32 ClientPosY = 0;
};