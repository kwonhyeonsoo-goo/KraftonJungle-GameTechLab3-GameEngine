//#include "SceneRenderCollector.h"
//#include "Component/UUIDBillboardComponent.h"
//#include "Renderer/RenderCommand.h"
//#include "Actor/Actor.h"
//#include "ShowFlags.h"
//void FSceneRenderCollector::CollectRenderCommands(const FFrustum& Frustum, FRenderCommandQueue& OutQueue)
//{
//
//	TArray<UPrimitiveComponent*> VisiblePrimitives;
//	FrustrumCull(Frustum, VisiblePrimitives);
//
//	for (UPrimitiveComponent* PrimitiveComponent : VisiblePrimitives)
//	{
//		if (!PrimitiveComponent)
//		{
//			continue;
//		}
//		if (PrimitiveComponent->IsA(UUUIDBillboardComponent::StaticClass()))
//		{
//
//			UUUIDBillboardComponent* UUIDComponent =
//				static_cast<UUUIDBillboardComponent*>(PrimitiveComponent);
//
//			FTextRenderCommand TextCmd;
//			TextCmd.Text = UUIDComponent->GetDisplayText();
//			TextCmd.WorldPosition = UUIDComponent->GetTextWorldPosition();
//			TextCmd.WorldScale = UUIDComponent->GetWorldScale();
//			TextCmd.Color = UUIDComponent->GetTextColor();
//
//			OutQueue.AddTextCommand(TextCmd);
//			continue;
//		}
//
//		if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData())
//		{
//			continue;
//		}
//
//		FRenderCommand Command;
//		Command.MeshData = PrimitiveComponent->GetPrimitive()->GetMeshData();
//		Command.WorldMatrix = PrimitiveComponent->GetWorldTransform();
//		Command.Material = PrimitiveComponent->GetMaterial();
//		OutQueue.AddCommand(Command);
//	}
//}
//
//void FSceneRenderCollector::FrustrumCull(const FFrustum& Frustum, TArray<UPrimitiveComponent*>& OutVisible)
//{
//	for (AActor* Actor : Actors)
//	{
//		if (!Actor || Actor->IsPendingDestroy())
//		{
//			continue;
//		}
//		if (!Actor->IsVisible())
//		{
//			continue;
//		}
//		for (UActorComponent* Component : Actor->GetComponents())
//		{
//
//			if (!Component->IsA(UPrimitiveComponent::StaticClass()))
//			{
//				continue;
//			}
//
//			UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);
//			// if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData())
//			if (PrimitiveComponent->IsA(UUUIDBillboardComponent::StaticClass()))
//			{
//				if (!ShowFlags.HasFlag(EEngineShowFlags::SF_BillboardText))
//				{
//					continue;
//				}
//			}
//			else
//			{
//				if (!ShowFlags.HasFlag(EEngineShowFlags::SF_Primitives))
//				{
//					continue;
//				}
//				if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData())
//				{
//					continue;
//				}
//			}
//
//			if (Frustum.IsVisible(PrimitiveComponent->GetWorldBounds()))
//			{
//				OutVisible.push_back(PrimitiveComponent);
//			}
//		}
//	}
//}
