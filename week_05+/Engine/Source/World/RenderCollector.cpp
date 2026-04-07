#include "RenderCollector.h"
#include "Component/UUIDBillboardComponent.h"
#include "Renderer/RenderCommand.h"
#include "Actor/Actor.h"
#include "Component/SubUVComponent.h"
#include "Core/FEngine.h"
#include "Component/TextComponent.h"
#include "Renderer/Renderer.h"
#include "Renderer/TextMeshBuilder.h"
#include "Renderer/SubUVRenderer.h"
#include "Renderer/Material.h"
#include "Component/StaticMeshComponent.h"
#include "Camera/Camera.h"

void FLevelRenderCollector::CollectRenderCommands(const TArray<AActor*>& Actors, const FFrustum& Frustum,
	const FShowFlags& ShowFlags, const FCamera* Camera, FRenderCommandQueue& OutQueue)
{
	TArray<UPrimitiveComponent*> VisiblePrimitives;
	FrustrumCull(Actors, Frustum, ShowFlags, VisiblePrimitives);

	if (!GRenderer) return;

	FTextMeshBuilder& TextRenderer = GRenderer->GetTextRenderer();
	FSubUVRenderer& SubUVRenderer = GRenderer->GetSubUVRenderer();

	for (UPrimitiveComponent* PrimitiveComponent : VisiblePrimitives)
	{
		if (!PrimitiveComponent) continue;

		// ─── 텍스트 컴포넌트 ───
		if (PrimitiveComponent->IsA(UTextComponent::StaticClass()))
		{
			UTextComponent* TextComp = static_cast<UTextComponent*>(PrimitiveComponent);
			FMeshData* TextMesh = TextComp->GetTextMesh();
			
			if (TextMesh && TextRenderer.BuildTextMesh(TextComp->GetDisplayText(), *TextMesh))
			{
				FMaterial* FontMat = TextRenderer.GetFontMaterial();
				if (FontMat)
				{
					FVector4 Color = TextComp->GetTextColor();
					FontMat->SetParameterData("TextColor", &Color, 16);

					FRenderCommand Command;
					Command.MeshData = TextMesh;
					Command.Material = FontMat;
					// TODO: UUID 렌더링 기능 재구현되면 아래 1줄 삭제
					if (!PrimitiveComponent->IsA(UUUIDBillboardComponent::StaticClass()))
					{
						Command.RenderLayer = ERenderLayer::Default;  // ← Overlay → Default
					}
					else
					{
						Command.RenderLayer = ERenderLayer::Overlay;
					}
				
#if IS_OBJ_VIEWER //뷰어에서는 텍스트, 빌보드 렌더링을 막습니다.
#else
					const FVector WorldPos = TextComp->GetRenderWorldPosition();
					const FVector Scale = TextComp->GetRenderWorldScale();

					if (TextComp->IsBillboard() && Camera)
					{
						const FVector CameraPos = Camera->GetPosition();
						Command.WorldMatrix = FMatrix::MakeScale(Scale) * FMatrix::MakeBillboard(WorldPos, CameraPos);
					}
					else
					{
						const float TextScale = TextComp->GetTextScale();
						Command.WorldMatrix =
							FMatrix::MakeScale(FVector(TextScale, TextScale, TextScale)) *
							TextComp->GetWorldTransform();
					}

					OutQueue.AddCommand(Command);
#endif
				}
			}
			continue;
		}

		// ─── SubUV 스프라이트 통합 ───
		// TODO: 일반적인 프리미티브와 RenderCommand build 경로 통합
		if (PrimitiveComponent->IsA(USubUVComponent::StaticClass()))
		{
			USubUVComponent* SubUVComponent = static_cast<USubUVComponent*>(PrimitiveComponent);
			FMeshData* SubUVMesh = SubUVComponent->GetSubUVMesh();
			if (SubUVMesh && SubUVRenderer.BuildSubUVMesh(SubUVComponent->GetSize(), *SubUVMesh))
			{
				float TotalTime = static_cast<float>(GEngine->GetCore()->GetTimer().GetTotalTime());
				SubUVRenderer.UpdateAnimationParams(
					SubUVComponent->GetColumns(), SubUVComponent->GetRows(), SubUVComponent->GetTotalFrames(),
					SubUVComponent->GetFirstFrame(), SubUVComponent->GetLastFrame(),
					SubUVComponent->GetFPS(), TotalTime, SubUVComponent->IsLoop()
				);

				FMaterial* SubUVMat = SubUVRenderer.GetSubUVMaterial();
				if (SubUVMat)
				{
					FRenderCommand Command;
					Command.MeshData = SubUVMesh;
					Command.Material = SubUVMat;
					Command.WorldMatrix = SubUVComponent->GetWorldTransform();

					if (SubUVComponent->IsBillboard() && Camera)
					{
						const FVector CameraPos = Camera->GetPosition();
						const FVector WorldPos = Command.WorldMatrix.GetTranslation();
						const FVector Scale = Command.WorldMatrix.GetScaleVector();
						Command.WorldMatrix = FMatrix::MakeScale(Scale) * FMatrix::MakeBillboard(WorldPos, CameraPos);
					}

					OutQueue.AddCommand(Command);
				}
			}
			continue;
		}
		if (PrimitiveComponent->IsA(UMeshComponent::StaticClass()))
		{
			UMeshComponent* MeshComp = static_cast<UMeshComponent*>(PrimitiveComponent);
			FMeshData* Data = MeshComp->GetMeshData();
			if (!Data) continue;
			const TArray<FMeshSection>& Sections = MeshComp->GetSections();

			for (const FMeshSection& Section : Sections)
			{
				FRenderCommand Command;
				Command.MeshData = Data;
				Command.FirstIndex = Section.FirstIndex;
				Command.IndexCount = Section.IndexCount;
				Command.Material = MeshComp->GetMaterial(Section.MaterialIndex);
				Command.WorldMatrix = MeshComp->GetWorldTransform();
				if (Command.Material && Command.Material->GetBlendOption().BlendEnable)
					Command.RenderLayer = ERenderLayer::Translucent;
				OutQueue.AddCommand(Command);
			}
			continue;
		}
		// ─── 일반 프리미티브 ───
		if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData())
		{
			continue;
		}

		FRenderCommand Command;
		Command.MeshData = PrimitiveComponent->GetPrimitive()->GetMeshData();
		Command.WorldMatrix = PrimitiveComponent->GetWorldTransform();
		Command.Material = PrimitiveComponent->GetMaterial();
		if (Command.Material && Command.Material->GetBlendOption().BlendEnable)
			Command.RenderLayer = ERenderLayer::Translucent;
		OutQueue.AddCommand(Command);
	}
}

void FLevelRenderCollector::FrustrumCull(const TArray<AActor*>& Actors, const FFrustum& Frustum,
	const FShowFlags& ShowFlags, TArray<UPrimitiveComponent*>& OutVisible)
{
	for (AActor* Actor : Actors)
	{
		if (!Actor || Actor->IsPendingDestroy()) continue;
		if (!Actor->IsVisible()) continue;

		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (!Component->IsA(UPrimitiveComponent::StaticClass())) continue;

			UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);

			const bool bIsUUID = PrimitiveComponent->IsA(UUUIDBillboardComponent::StaticClass());
			const bool bIsSubUV = PrimitiveComponent->IsA(USubUVComponent::StaticClass());
			const bool bIsText = PrimitiveComponent->IsA(UTextComponent::StaticClass());
			const bool bIsMeshComp = PrimitiveComponent->IsA(UMeshComponent::StaticClass());
			if (bIsUUID)
			{
				if (!ShowFlags.HasFlag(EEngineShowFlags::SF_UUID)) continue;
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
			else if (bIsMeshComp)
			{
				if (!ShowFlags.HasFlag(EEngineShowFlags::SF_Primitives)) continue;
				UMeshComponent* MC = static_cast<UMeshComponent*>(PrimitiveComponent);
				if (!MC->GetMeshData()) continue;
			}
			else
			{
				if (!ShowFlags.HasFlag(EEngineShowFlags::SF_Primitives)) continue;
				if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData()) continue;
			}

			if (bIsMeshComp)
			{
				UMeshComponent* MC = static_cast<UMeshComponent*>(PrimitiveComponent);
				FBoxSphereBounds Bounds = MC->GetWorldBounds();
				if (Frustum.IsVisible(Bounds))
					OutVisible.push_back(PrimitiveComponent);
			}
			else if (Frustum.IsVisible(PrimitiveComponent->GetWorldBounds()))
			{
				OutVisible.push_back(PrimitiveComponent);
			}
		}
	}
}
