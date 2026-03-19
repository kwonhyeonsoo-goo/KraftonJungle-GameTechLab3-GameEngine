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
	PushGrid(queue, session.GetActiveCamera().Position);
	PushWorldAxis(queue);
	const TArray<USceneComponent*> primary = session.Selection.GetAll();
	if (primary.empty())
		return;

	PushHighlight(primary, queue);

	ITool* activeTool = ctx.Editor.Tools.GetActiveTool();
	if (!activeTool) return;

	GizmoState State;
	activeTool->FillGizmoState(ctx, State);
	if (State.bActive)
		PushGizmoAxes(State, queue);
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

void OverlayBuilder::PushHighlight(const TArray<USceneComponent*> objs, RenderQueue& queue)
{
	for(auto comp : objs)
	{
		RenderCommand highlightCommand = {};
		highlightCommand.Type = ERenderType::Highlight;
		highlightCommand.Shape = comp->IsA<UPrimitiveComponent>() ? static_cast<const UPrimitiveComponent*>(comp)->Shape : EPrimitiveShape::Cube;
		highlightCommand.WorldTransform = comp->GetWorldMatrix();
		highlightCommand.Color = 0xFFFFFF00; // Yellow with full opacity
		highlightCommand.ObjectId = comp->GetUUID();
		queue.Push(highlightCommand);
	}
}

void OverlayBuilder::PushGrid(RenderQueue& queue , FVector cameraPos)
{
	RenderCommand axisCommand = {};
	axisCommand.Type = ERenderType::Grid;
	axisCommand.Shape;
	FVector cameraPosXY = { cameraPos.x, cameraPos.y, 0 };
	axisCommand.WorldTransform = FMatrix::Translation(cameraPosXY);

	axisCommand.Color = 0;
	axisCommand.ObjectId = 0;

	queue.Push(axisCommand);

}

void OverlayBuilder::PushGizmoAxes(const GizmoState& state, RenderQueue& queue)
{
	const bool IsUniformScale = state.bUniformScale;
	const bool IsActive = (state.HoveredAxisIndex >= 0 || state.ActiveAxisIndex >= 0);

	for (int i = 0; i < 3; ++i)
	{
		const bool bHighlight = (i == state.HoveredAxisIndex) || (i == state.ActiveAxisIndex) || (IsUniformScale && IsActive);

		RenderCommand cmd = {};
		cmd.Type           = state.Axes[i].RenderType;
		cmd.WorldTransform = state.Axes[i].WorldTransform;
		cmd.Color          = bHighlight ? AxisColorHighlight : state.Axes[i].BaseColor;
		queue.Push(cmd);
	}
}
