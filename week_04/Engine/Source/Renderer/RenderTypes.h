#pragma once

struct FFrameRenderParams
{
	/** 추후 GameTime, ... 등으로 분리가 필요할 것으로 보임 */
	/** 현재는 Core 의 FTimer 의 Total Time */
	float Time = 0;
	float UVScrollVelocity[2] = { 0, 0 };
};
