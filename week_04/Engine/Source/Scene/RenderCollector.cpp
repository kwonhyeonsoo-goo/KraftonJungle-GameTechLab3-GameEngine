#include "RenderCollector.h"
#include "Component/UUIDBillboardComponent.h"
#include "Renderer/RenderCommand.h"
#include "Actor/Actor.h"
#include "Component/SubUVComponent.h"
#include "Core/FEngine.h"
#include "Component/TextComponent.h"


void FSceneRenderCollector::CollectRenderCommands(const TArray<AActor*>& Actors, const FFrustum& Frustum,
	const FShowFlags& ShowFlags, FRenderCommandQueue& OutQueue)
{
	TArray<UPrimitiveComponent*> VisiblePrimitives;
	FrustrumCull(Actors, Frustum, ShowFlags, VisiblePrimitives);

	for (UPrimitiveComponent* PrimitiveComponent : VisiblePrimitives)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}
		if (PrimitiveComponent->IsA(UUUIDBillboardComponent::StaticClass()))
		{
			UUUIDBillboardComponent* UUIDComponent =
				static_cast<UUUIDBillboardComponent*>(PrimitiveComponent);

			FTextRenderCommand TextCmd;
			TextCmd.Text = UUIDComponent->GetDisplayText();
			FVector TextPos = UUIDComponent->GetTextWorldPosition();
			TextCmd.WorldMatrix = FMatrix::MakeTranslation(TextPos);

			// 카메라 거리에 비례해서 스케일 보정 -> 화면 크기 일정
			FVector CameraPos = OutQueue.ViewMatrix.GetInverse().GetTranslation();
			float Distance = (TextPos - CameraPos).Size();
			float BaseDistance = 5.0f;
			float DistanceScale = (Distance > BaseDistance) ? (Distance / BaseDistance) : 1.0f;

			const float Scale = UUIDComponent->GetWorldScale() * DistanceScale;
			TextCmd.WorldScale = FVector(Scale, Scale, Scale);
			TextCmd.bBillboard = true;
			TextCmd.Color = UUIDComponent->GetTextColor();

			OutQueue.AddTextCommand(TextCmd);
			continue;
		}

		if (PrimitiveComponent->IsA(UTextComponent::StaticClass()))
		{
			UTextComponent* TextComponent = static_cast<UTextComponent*>(PrimitiveComponent);

			FTextRenderCommand TextCmd;
			TextCmd.Text = TextComponent->GetText();
			TextCmd.WorldMatrix = TextComponent->GetWorldTransform();
			TextCmd.WorldScale = TextComponent->GetWorldTransform().GetScaleVector();			
			TextCmd.bBillboard = TextComponent->IsBillboard();
			TextCmd.Color = TextComponent->GetTextColor();

			OutQueue.AddTextCommand(TextCmd);
			continue;
		}

		if (PrimitiveComponent->IsA(USubUVComponent::StaticClass()))
		{
			USubUVComponent* SubUVComponent = static_cast<USubUVComponent*>(PrimitiveComponent);

			FSubUVRenderCommand SubUVCmd;
			SubUVCmd.WorldMatrix = SubUVComponent->GetWorldTransform();
			SubUVCmd.Size = SubUVComponent->GetSize();
			SubUVCmd.Columns = SubUVComponent->GetColumns();
			SubUVCmd.Rows = SubUVComponent->GetRows();
			SubUVCmd.TotalFrames = SubUVComponent->GetTotalFrames();
			SubUVCmd.FPS = SubUVComponent->GetFPS();
			SubUVCmd.ElapsedTime = static_cast<float>(GEngine->GetCore()->GetTimer().GetTotalTime());
			SubUVCmd.bLoop = SubUVComponent->IsLoop();
			SubUVCmd.bBillboard = SubUVComponent->IsBillboard();
			SubUVCmd.FirstFrame = SubUVComponent->GetFirstFrame();
			SubUVCmd.LastFrame = SubUVComponent->GetLastFrame();

			OutQueue.AddSubUVCommand(SubUVCmd);
			continue;
		}

		if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData())
		{
			continue;
		}

		FRenderCommand Command;
		Command.MeshData = PrimitiveComponent->GetPrimitive()->GetMeshData();
		Command.WorldMatrix = PrimitiveComponent->GetWorldTransform();
		Command.Material = PrimitiveComponent->GetMaterial();
		OutQueue.AddCommand(Command);
	}
}

void FSceneRenderCollector::FrustrumCull(const TArray<AActor*>& Actors, const FFrustum& Frustum,
	const FShowFlags& ShowFlags, TArray<UPrimitiveComponent*>& OutVisible)
{
	for (AActor* Actor : Actors)
	{
		if (!Actor || Actor->IsPendingDestroy())
		{
			continue;
		}
		if (!Actor->IsVisible())
		{
			continue;
		}
		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (!Component->IsA(UPrimitiveComponent::StaticClass()))
			{
				continue;
			}

			UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);

			const bool bIsUUID = PrimitiveComponent->IsA(UUUIDBillboardComponent::StaticClass());
			const bool bIsSubUV = PrimitiveComponent->IsA(USubUVComponent::StaticClass());
			const bool bIsText = PrimitiveComponent->IsA(UTextComponent::StaticClass());

			if (bIsUUID)
			{
				if (!ShowFlags.HasFlag(EEngineShowFlags::SF_UUID))
				{
					continue;
				}
			}
			else if (bIsSubUV)
			{
				if (!ShowFlags.HasFlag(EEngineShowFlags::SF_Billboard))
				{
					continue;
				}
			}
			else if (bIsText)
			{
				if (!ShowFlags.HasFlag(EEngineShowFlags::SF_Text))
				{
					continue;
				}
			}
			else
			{
				if (!ShowFlags.HasFlag(EEngineShowFlags::SF_Primitives))
				{
					continue;
				}

				if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData())
				{
					continue;
				}
			}

			if (Frustum.IsVisible(PrimitiveComponent->GetWorldBounds()))
			{
				OutVisible.push_back(PrimitiveComponent);
			}
		}  // for Component
	}  // for Actor
}  // FrustrumCull ??