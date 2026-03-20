#include "PhysicsManager.h"
#include "Scene/Scene.h"
#include "Math/Vector.h"
#include "Actor/Actor.h"
#include "Component/PrimitiveComponent.h"

bool CPhysicsManager::Linetrace(const UScene* Scene, const FVector& Start, const FVector& End, FHitResult& OutHit)
{
	const TArray<AActor*>& Actors = Scene->GetActors();

	FVector LineDirection = End - Start;
	LineDirection.Normalize();

	// Normalize 실패 상황 고려
	if (!LineDirection.IsZero())
	{
		for (AActor* Actor : Actors)
		{
			if (!Actor || Actor->IsPendingDestroy())
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
				if (!PrimitiveComponent->GetPrimitive() || !PrimitiveComponent->GetPrimitive()->GetMeshData())
				{
					continue;
				}

				FBoxSphereBounds Bound = PrimitiveComponent->GetWorldBoundsForAABB();

				FVector VecToOrigin = Bound.Center - Start;
				float ShortestT = FVector::DotProduct(VecToOrigin, LineDirection);
				FVector ShortestPos = Start + LineDirection * ShortestT;
				float ShortestDistSquared = (ShortestPos - Bound.Center).SizeSquared();

				// 빠른 검사를 위해 일차적으로 Sphere 로 test
				if (ShortestDistSquared <= Bound.RadiusSquared)
				{
					// 정밀한 검사를 위해 Box test
					FVector SlabMin = Bound.Center - Bound.BoxExtent;
					FVector SlabMax = Bound.Center + Bound.BoxExtent;

					FVector DirectionInv(1 / LineDirection.X, 1 / LineDirection.Y, 1 / LineDirection.Z);

					FVector T1 = FVector::Multiply((SlabMin - Start), DirectionInv);
					FVector T2 = FVector::Multiply((SlabMax - Start), DirectionInv);

					FVector TMinVec = FVector::Min(T1, T2);
					FVector TMaxVec = FVector::Max(T1, T2);

					float tNear = FMath::Max(FMath::Max(TMinVec.X, TMinVec.Y), TMinVec.Z);
					float tFar = FMath::Min(FMath::Min(TMaxVec.X, TMaxVec.Y), TMaxVec.Z);

					bool bIntersected =
						(tNear <= tFar) &&
						(tFar >= 0.0f) &&
						(tNear * tNear <= (End - Start).SizeSquared());

					if (bIntersected)
					{
						UE_LOG("Actor %s line collided", Actor->GetName());
						OutHit.HitActor = Actor;
						return true;
					}
				}
				else
				{
					// rejct
				}

			}
		}
	}
	
	return false;
}
