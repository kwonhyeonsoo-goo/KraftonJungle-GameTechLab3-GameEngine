#include "OverlayBuilder.h"
#include "RenderTypes.h"
#include "../Foundation/Math/FMatrix.h"
#include "RenderQueue.h"
#include "../../AppContext.h"
#include "../../Engine/World/UPrimitiveComponent.h"
#include "../World/Transform.h"
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

void OverlayBuilder::Build(const EditorSession& session, AppContext& ctx, RenderQueue& queue)

{
	// Default implementation: add world axis command to the render queue.
	// Implemented inline so the definition is available to all TUs and
	// to avoid linker unresolved external when the .cpp is not linked.
	PushGrid(queue);
	PushWorldAxis(queue);
	USceneComponent* primary = session.Selection.GetPrimary();
	if (primary == nullptr)
		return;

	PushHighlight(primary, queue);

	ITool* activeTool = ctx.Editor.Tools.GetActiveTool();
	if (activeTool != nullptr)
	{
		activeTool->BuildGizmoOverlay(ctx, queue);
	}


}


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
	RenderCommand axisCommand = {};
	axisCommand.Type = ERenderType::Highlight;
	axisCommand.Shape = static_cast<const UPrimitiveComponent*>(comp)->Shape;

	axisCommand.WorldTransform = comp->GetWorldMatrix();

	axisCommand.Color = 0;
	axisCommand.ObjectId = comp->GetUUID();

	queue.Push(axisCommand);

}

void OverlayBuilder::PushGrid(RenderQueue& queue)
{
	RenderCommand axisCommand = {};
	axisCommand.Type = ERenderType::Grid;
	axisCommand.Shape;

	axisCommand.WorldTransform = FMatrix::FMatrix();

	axisCommand.Color = 0;
	axisCommand.ObjectId = 0;

	queue.Push(axisCommand);

}
