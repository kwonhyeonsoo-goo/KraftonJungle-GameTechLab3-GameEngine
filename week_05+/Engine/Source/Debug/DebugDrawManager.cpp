#include "DebugDrawManager.h"
#include "Renderer/Renderer.h"
#include "Core/ShowFlags.h"
#include "World/World.h"
#include "Actor/Actor.h"
#include "Component/PrimitiveComponent.h"
#include "Component/UUIDBillboardComponent.h"
#include "Component/SubUVComponent.h"
#include "Component/MeshComponent.h"
#include "Object/Class.h"
void FDebugDrawManager::DrawLine(const FVector& Start, const FVector& End, const FVector4& Color)
{
	Lines.push_back({ Start, End, Color });
}

void FDebugDrawManager::DrawCube(const FVector& Center, const FVector& Extent, const FVector4& Color)
{
	Cubes.push_back({ Center, Extent, Color });
}

void FDebugDrawManager::DrawWorldAxis(float Length)
{
	bDrawWorldAxis = true;
	WorldAxisLength = Length;
}

void FDebugDrawManager::Flush(FRenderer* Renderer, const FShowFlags& ShowFlags, UWorld* World)
{
	if (!Renderer)
	{
		return;
	}

	const bool bShowDebugDraw = ShowFlags.HasFlag(EEngineShowFlags::SF_DebugDraw);
	const bool bShowWorldAxis = ShowFlags.HasFlag(EEngineShowFlags::SF_WorldAxis) || bDrawWorldAxis;

	if (!bShowDebugDraw && !bShowWorldAxis)
	{
		Clear();
		return;
	}

	if (bShowDebugDraw && ShowFlags.HasFlag(EEngineShowFlags::SF_Collision) && World)
	{
		DrawAllCollisionBounds(Renderer, World);
	}

	if (bShowDebugDraw)
	{
		for (const auto& Cube : Cubes)
		{
			Renderer->DrawCube(Cube.Center, Cube.Extent, Cube.Color);
		}
		for (const auto& Line : Lines)
		{
			Renderer->DrawLine(Line.Start, Line.End, Line.Color);
		}
	}

	if (bShowWorldAxis)
	{
		Renderer->DrawLine({ 0.0f, 0.0f, 0.0f }, { WorldAxisLength, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
		Renderer->DrawLine({ 0.0f, 0.0f, 0.0f }, { 0.0f, WorldAxisLength, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
		Renderer->DrawLine({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, WorldAxisLength }, { 0.0f, 0.0f, 1.0f, 1.0f });
	}

	Renderer->ExecuteLineCommands();
	Clear();
}

void FDebugDrawManager::Clear()
{
	Lines.clear();
	Cubes.clear();
	bDrawWorldAxis = false;
	WorldAxisLength = 1000.0f;
}

void FDebugDrawManager::DrawAllCollisionBounds(FRenderer* Renderer, UWorld* World)
{
	TArray<AActor*> AllActors = World->GetAllActors();
	for (AActor* Actor : AllActors)
	{
		if (!Actor || Actor->IsPendingDestroy() || !Actor->IsVisible())
			continue;

		for (UActorComponent* Comp : Actor->GetComponents())
		{
			if (!Comp->IsA(UPrimitiveComponent::StaticClass()))
				continue;

			UPrimitiveComponent* PrimComp = static_cast<UPrimitiveComponent*>(Comp);

			// 빌보드, SubUV 제외
			if (!PrimComp->ShouldDrawDebugBounds()) continue;

			bool bHasValidMesh = false;

			if (PrimComp->IsA(UMeshComponent::StaticClass()))
			{
				bHasValidMesh = static_cast<UMeshComponent*>(PrimComp)->GetMeshData() != nullptr;
			}
			else if (PrimComp->GetPrimitive() && PrimComp->GetPrimitive()->GetMeshData())
			{
				bHasValidMesh = true;
			}

			if (!bHasValidMesh)
			{
				continue;
			}

			FBoxSphereBounds Bound = PrimComp->GetWorldBounds();
			Renderer->DrawCube(Bound.Center, Bound.BoxExtent, FVector4(1, 0, 0, 1));  // 초록색
		}
	}
}
