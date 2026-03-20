#pragma once

class AActor;
class UScene;
struct FVector;

struct FHitResult
{
	AActor* HitActor;
};

class CPhysicsManager
{
public:
	/**
	 * 처음으로 Hit 된 대상에 대한 정보를 OutHit 로 반환.
	 * 
	 * \return Hit 성공 여부
	 */
	bool Linetrace(const UScene* Scene, const FVector& Start, const FVector& End, FHitResult& OutHit);
private:
};
