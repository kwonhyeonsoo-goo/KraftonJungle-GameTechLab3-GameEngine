#include "RenderCollector.h"
#include "Component/BillboardComponent.h"
#include "Renderer/RenderCommand.h"
#include "Actor/Actor.h"
#include "Component/SubUVComponent.h"
#include "Core/FEngine.h"
#include "Component/TextRenderComponent.h"
#include "Renderer/Renderer.h"
#include "Renderer/TextMeshBuilder.h"
#include "Renderer/SubUVRenderer.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/Material.h"
#include "Component/StaticMeshComponent.h"
#include "Camera/Camera.h"

void FLevelRenderCollector::CollectRenderCommands(const TArray<AActor*>& Actors, const FFrustum& Frustum,
	const FShowFlags& ShowFlags, const FCamera* Camera, FRenderCommandQueue& OutQueue)
{
	extern ENGINE_API FRenderer* GRenderer;
	if (GRenderer && GRenderer->IsInstanceBufferDirty())
	{
		bCommandsDirty = true;
	}
	bool bFrustumChanged = !PrevFrustum.Equals(Frustum);
	bool bShowFlagsChanged = (PrevShowFlagsBits != ShowFlags.GetFlags());
	bool bActorCountChanged = (PrevActorCount != Actors.size());

	if (!bCommandsDirty && !bFrustumChanged && !bShowFlagsChanged && !bActorCountChanged && !bHasAnimatedComponents)
	{
		OutQueue.Commands = CachedQueue.Commands;
		return;
	}

	TArray<UPrimitiveComponent*> VisiblePrimitives;
	FrustrumCull(Actors, Frustum, ShowFlags, VisiblePrimitives);

	if (!GRenderer) return;

	OutQueue.Clear();
	bHasAnimatedComponents = false;

	FTextMeshBuilder& TextRenderer = GRenderer->GetTextRenderer();
	FSubUVRenderer& SubUVRenderer = GRenderer->GetSubUVRenderer();

	for (UPrimitiveComponent* PrimitiveComponent : VisiblePrimitives)
	{
		if (!PrimitiveComponent) continue;

		// ─── 1. 텍스트 컴포넌트 (UUID 포함) ───
		if (PrimitiveComponent->IsA(UTextRenderComponent::StaticClass()))
		{
			UTextRenderComponent* TextComp = static_cast<UTextRenderComponent*>(PrimitiveComponent);
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

					// 텍스트는 오브젝트를 뚫고 보이도록 Overlay 처리
					Command.RenderLayer = ERenderLayer::Overlay;

#if !IS_OBJ_VIEWER // 뷰어가 아닐 때만 텍스트 렌더링
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

					//  피킹/배칭 데이터 누락 수정
					if (AActor* OwnerActor = PrimitiveComponent->GetOwner()) Command.ObjectID = OwnerActor->GetUUID();
					Command.SortKey = FRenderCommand::MakeSortKey(Command.Material, Command.MeshData, 0);

					OutQueue.AddCommand(Command);
#endif
				}
			}
			continue;
		}

		// ─── 2. 빌보드 컴포넌트 (에디터 아이콘, 새로 추가됨) ───
		if (PrimitiveComponent->IsA(UBillboardComponent::StaticClass()))
		{
			UBillboardComponent* BillComp = static_cast<UBillboardComponent*>(PrimitiveComponent);
			FMeshData* BillMesh = BillComp->GetBillboardMesh();

			if (BillMesh)
			{
				if (BillMesh->IsDirty() || BillMesh->VertexBuffer == nullptr)
				{
					BillMesh->UpdateVertexAndIndexBuffer(GRenderer->GetDevice(), GRenderer->GetDeviceContext());
					BillMesh->bIsDirty = false;
				}
				FRenderCommand Command;
				Command.MeshData = BillMesh;
				Command.Material = BillComp->GetMaterial();
				Command.RenderLayer = ERenderLayer::Default;
				if (!Command.Material)
				{
					if (auto DefaultMat = FMaterialManager::Get().FindByName("M_Default_Texture"))
						Command.Material = DefaultMat.get();
				}

		
				if (!Command.Material) continue;

#if !IS_OBJ_VIEWER // 뷰어가 아닐 때만 아이콘 렌더링
				const FVector2& Size = BillComp->GetSize();
				const FVector WorldPos = BillComp->GetWorldLocation();
				const FVector CurrentScale = BillComp->GetWorldTransform().GetScaleVector();
				FVector FinalScale(CurrentScale.X * Size.X, CurrentScale.Y * Size.Y, CurrentScale.Z);

				if (Camera)
				{
					Command.WorldMatrix = FMatrix::MakeScale(FinalScale) * FMatrix::MakeBillboard(WorldPos, Camera->GetPosition());
				}

				if (AActor* OwnerActor = PrimitiveComponent->GetOwner()) Command.ObjectID = OwnerActor->GetUUID();
				Command.SortKey = FRenderCommand::MakeSortKey(Command.Material, Command.MeshData, 0);

				OutQueue.AddCommand(Command);
#endif
			}
			continue;
		}

		// ─── 3. SubUV 스프라이트 ───
		if (PrimitiveComponent->IsA(USubUVComponent::StaticClass()))
		{
			bHasAnimatedComponents = true;
			USubUVComponent* SubUVComponent = static_cast<USubUVComponent*>(PrimitiveComponent);
			FMeshData* SubUVMesh = SubUVComponent->GetSubUVMesh();
			if (SubUVMesh && SubUVRenderer.BuildSubUVMesh(SubUVComponent->GetSize(), *SubUVMesh))
			{
				float TotalTime = static_cast<float>(GEngine->GetTimer()->GetTotalTime());
				SubUVRenderer.UpdateAnimationParams(
					SubUVComponent->GetColumns(), SubUVComponent->GetRows(), SubUVComponent->GetTotalFrames(),
					SubUVComponent->GetFirstFrame(), SubUVComponent->GetLastFrame(),
					SubUVComponent->GetFPS(), TotalTime, SubUVComponent->IsLoop(),
					SubUVComponent->IsPreview()
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

					//  피킹/배칭 데이터 누락 수정
					if (AActor* OwnerActor = PrimitiveComponent->GetOwner()) Command.ObjectID = OwnerActor->GetUUID();
					Command.SortKey = FRenderCommand::MakeSortKey(Command.Material, Command.MeshData, 0);

					OutQueue.AddCommand(Command);
				}
			}
			continue;
		}

		// ─── 4. Mesh 컴포넌트 ───
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
				Command.SortKey = FRenderCommand::MakeSortKey(Command.Material, Command.MeshData, Command.FirstIndex);

				if (Command.Material && Command.Material->GetBlendOption().BlendEnable)
					Command.RenderLayer = ERenderLayer::Translucent;

				if (AActor* OwnerActor = PrimitiveComponent->GetOwner())
				{
					Command.ObjectID = OwnerActor->GetUUID();
				}
				OutQueue.AddCommand(Command);
			}
			continue;
		}

		// ─── 5. 일반 프리미티브 ───
		if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData())
		{
			continue;
		}

		FRenderCommand Command;
		Command.MeshData = PrimitiveComponent->GetPrimitive()->GetMeshData();
		Command.WorldMatrix = PrimitiveComponent->GetWorldTransform();
		Command.Material = PrimitiveComponent->GetMaterial();
		if (!Command.Material)
		{
			if (auto DefaultMat = FMaterialManager::Get().FindByName("M_Default"))
				Command.Material = DefaultMat.get();
		}
		if (AActor* OwnerActor = PrimitiveComponent->GetOwner())
		{
			Command.ObjectID = OwnerActor->GetUUID();
		}
		Command.SortKey = FRenderCommand::MakeSortKey(Command.Material, Command.MeshData, 0);

		if (Command.Material && Command.Material->GetBlendOption().BlendEnable)
			Command.RenderLayer = ERenderLayer::Translucent;

		OutQueue.AddCommand(Command);
	}

	CachedQueue.Commands = OutQueue.Commands;
	PrevFrustum = Frustum;
	PrevShowFlagsBits = ShowFlags.GetFlags();
	PrevActorCount = Actors.size();
	bCommandsDirty = false;
}

void FLevelRenderCollector::FrustrumCull(const TArray<AActor*>& Actors, const FFrustum& Frustum,
	const FShowFlags& ShowFlags, TArray<UPrimitiveComponent*>& OutVisible)
{
	for (AActor* Actor : Actors)
	{
		if (!Actor || Actor->IsPendingDestroy() || !Actor->IsVisible()) continue;

		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (!Component->IsA(UPrimitiveComponent::StaticClass())) continue;

			UPrimitiveComponent* PrimitiveComponent = static_cast<UPrimitiveComponent*>(Component);

			//  상속 관계가 분리되었으므로 명확하게 변수 세팅
			const bool bIsTextureBillboard = PrimitiveComponent->IsA(UBillboardComponent::StaticClass());
			const bool bIsSubUV = PrimitiveComponent->IsA(USubUVComponent::StaticClass());
			const bool bIsText = PrimitiveComponent->IsA(UTextRenderComponent::StaticClass()) && !bIsTextureBillboard;
			const bool bIsMeshComp = PrimitiveComponent->IsA(UMeshComponent::StaticClass());

			// 플래그 검사 (텍스처 빌보드와 SubUV는 SF_Billboard를 따름)
			if (bIsTextureBillboard || bIsSubUV)
			{
				if (!ShowFlags.HasFlag(EEngineShowFlags::SF_Billboard)) continue;
			}
			else if (bIsText)
			{
				UTextRenderComponent* TextComp = static_cast<UTextRenderComponent*>(PrimitiveComponent);
				// 텍스트 내용이 비어있으면 UUID 역할을 하므로 SF_UUID 스위치를 따름
				if (TextComp->GetText().empty())
				{
					if (!ShowFlags.HasFlag(EEngineShowFlags::SF_UUID)) continue;
				}
				else
				{
					if (!ShowFlags.HasFlag(EEngineShowFlags::SF_Text)) continue;
				}
			}
			else if (bIsMeshComp)
			{
				if (!ShowFlags.HasFlag(EEngineShowFlags::SF_Primitives)) continue;
				UMeshComponent* MC = static_cast<UMeshComponent*>(PrimitiveComponent);
				if (!MC->GetMeshData()) continue;
			}
			else // 일반 도형
			{
				if (!ShowFlags.HasFlag(EEngineShowFlags::SF_Primitives) && !ShowFlags.HasFlag(EEngineShowFlags::SF_Collision))
					continue;

				if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData()) 
					continue;
			}

			// 바운드 박스 컬링
			if (bIsMeshComp)
			{
				UMeshComponent* MC = static_cast<UMeshComponent*>(PrimitiveComponent);
				if (Frustum.IsVisible(MC->GetWorldBounds()))
					OutVisible.push_back(PrimitiveComponent);
			}
			else if (Frustum.IsVisible(PrimitiveComponent->GetWorldBounds()))
			{
				OutVisible.push_back(PrimitiveComponent);
			}
		}
	}
}