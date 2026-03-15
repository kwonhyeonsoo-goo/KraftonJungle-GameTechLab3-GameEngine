#pragma once
#include "RenderSceneExtractor.h"
#include "RenderQueue.h"
#include "../ObjectKernel/UObject.h"
#include "../Foundation/Core/CoreTypes.h"
#include "../Foundation/Math/FMatrix.h"
#include "../Foundation/Math/FVector.h"
#include "../../AppContext.h"

/*


struct RenderCommand {
	ERenderType     Type;
	EPrimitiveShape Shape;          // Primitive/Highlight만 유효
	FMatrix        WorldTransform;
	uint32_t          Color;          // 0xRRGGBBAA
	uint32_t         ObjectId;       // Picking 역참조, 디버그용
};


*/
void RenderSceneExtractor::Extract(const AppContext& ctx, RenderQueue& queue)
{
	TArray<UObject*> objects = ctx.Objects.GUObjectArray();


	for (UObject* o : objects) {
		if (o->IsA<USceneComponent>()) {
			RenderCommand objectRenderCommand = {};
			objectRenderCommand.Type = ERenderType::Primitive;

			USceneComponent* sceneComponent = static_cast<USceneComponent*>(o);

			objectRenderCommand.Shape = static_cast<UPrimitiveComponent*>(o)->Shape;

			objectRenderCommand.WorldTransform = sceneComponent->GetWorldMatrix();

			objectRenderCommand.Color = 0;

			objectRenderCommand.ObjectId = o->GetUUID();

			queue.Push(objectRenderCommand);
		}
	}
}
