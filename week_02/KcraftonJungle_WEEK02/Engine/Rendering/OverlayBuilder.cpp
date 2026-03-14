#include "OverlayBuilder.h"
#include "RenderTypes.h"
#include "../Foundation/Math/FMatrix.h"
#include "RenderQueue.h"
/*


struct RenderCommand {
	ERenderType     Type;
	EPrimitiveShape Shape;          // Primitive/Highlight만 유효
	FMatrix        WorldTransform;
	uint32_t          Color;          // 0xRRGGBBAA
	uint32_t         ObjectId;       // Picking 역참조, 디버그용
};


*/

// Build implementation moved inline into header to ensure linker visibility.

void OverlayBuilder::PushWorldAxis(RenderQueue& queue)
{
	RenderCommand axisCommand = {};
	axisCommand.Type = ERenderType::WorldAxis;
	axisCommand.Shape;
	axisCommand.WorldTransform = {
		 1.f, 0.f, 0.f, 0.f ,
		 0.f, 1.f, 0.f, 0.f ,
		 0.f, 0.f, 1.f, 0.f ,
		 0.f, 0.f, 0.f, 1.f };
	axisCommand.Color = 0;
	axisCommand.ObjectId = 0;

	queue.Push(axisCommand);

}

void OverlayBuilder::PushLocalAxis(const USceneComponent* comp, RenderQueue& queue)
{
}

void OverlayBuilder::PushHighlight(const USceneComponent* comp, RenderQueue& queue)
{
}

void OverlayBuilder::PushGrid(RenderQueue& queue)
{
}
