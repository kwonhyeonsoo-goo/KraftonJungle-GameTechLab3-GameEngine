#pragma once

#include "Physics/IPhysicsScene.h"
#include "Core/Types/CollisionTypes.h"
#include "Math/Vector.h"
#include "Object/Ptr/WeakObjectPtr.h"
#include "Object/Object.h"
#include <unordered_set>
#include <unordered_map>
#include <vector>

struct FNativePhysicsPair
{
    uint32                              AId = 0;
    uint32                              BId = 0;
    TWeakObjectPtr<UPrimitiveComponent> A;
    TWeakObjectPtr<UPrimitiveComponent> B;

    FNativePhysicsPair() = default;
    FNativePhysicsPair(UPrimitiveComponent* InA, UPrimitiveComponent* InB);

    bool IsValid() const
    {
        return AId != 0 && BId != 0 && AId != BId;
    }

    bool operator==(const FNativePhysicsPair& Other) const
    {
        return AId == Other.AId && BId == Other.BId;
    }
};

namespace std
{
    template <>
    struct hash<FNativePhysicsPair>
    {
        size_t operator()(const FNativePhysicsPair& Pair) const
        {
            size_t H = hash<uint32>()(Pair.AId);
            H        ^= hash<uint32>()(Pair.BId) + 0x9e3779b9 + (H << 6) + (H >> 2);
            return H;
        }
    };
}

// ============================================================
// FNativePhysicsScene — 기존 hand-written 충돌 시스템 래핑
//
// O(N²) brute-force + CollisionMath + 채널/응답 필터링.
// IPhysicsScene 인터페이스를 통해 교체 가능.
// ============================================================
class FNativePhysicsScene : public IPhysicsScene
{
public:
	void Initialize(UWorld* InWorld) override;
	void Shutdown() override;

	void RegisterComponent(UPrimitiveComponent* Comp) override;
	void UnregisterComponent(UPrimitiveComponent* Comp) override;
	void RebuildBody(UPrimitiveComponent* Comp) override;

	void Tick(float DeltaTime) override;

	void AddForce(UPrimitiveComponent* Comp, const FVector& Force) override;
	void AddForceAtLocation(UPrimitiveComponent* Comp, const FVector& Force, const FVector& WorldLocation) override;
	void AddTorque(UPrimitiveComponent* Comp, const FVector& Torque) override;

	FVector GetLinearVelocity(UPrimitiveComponent* Comp) const override;
	void SetLinearVelocity(UPrimitiveComponent* Comp, const FVector& Vel) override;
	FVector GetAngularVelocity(UPrimitiveComponent* Comp) const override;
	void SetAngularVelocity(UPrimitiveComponent* Comp, const FVector& Vel) override;

	void SetMass(UPrimitiveComponent* Comp, float Mass) override;
	float GetMass(UPrimitiveComponent* Comp) const override;
	void SetCenterOfMass(UPrimitiveComponent* Comp, const FVector& LocalOffset) override;
	FVector GetCenterOfMass(UPrimitiveComponent* Comp) const override;

	void SetLinearLock(UPrimitiveComponent* Comp, bool bX, bool bY, bool bZ) override;
	void SetAngularLock(UPrimitiveComponent* Comp, bool bX, bool bY, bool bZ) override;

	bool Raycast(const FVector& Start, const FVector& Dir, float MaxDist, FHitResult& OutHit,
		ECollisionChannel TraceChannel = ECollisionChannel::WorldStatic,
		const AActor* IgnoreActor = nullptr) const override;

	bool RaycastByObjectTypes(const FVector& Start, const FVector& Dir, float MaxDist, FHitResult& OutHit,
		uint32 ObjectTypeMask, const AActor* IgnoreActor = nullptr) const override;

private:
	void CompactInvalidComponents();
    void ErasePairsWithInvalidComponents(std::unordered_set<FNativePhysicsPair>& Pairs);
    void ErasePairsWithComponent(std::unordered_set<FNativePhysicsPair>& Pairs, UPrimitiveComponent* Comp);

	UWorld*                                          World = nullptr;
    std::vector<TWeakObjectPtr<UPrimitiveComponent>> RegisteredComponents;

	// 물리 시뮬레이션 상태
	struct FBodyState
	{
		FVector Velocity = { 0, 0, 0 };
		FVector AngularVelocity = { 0, 0, 0 };
		FVector AccumulatedForce = { 0, 0, 0 };
		FVector AccumulatedTorque = { 0, 0, 0 };
		float Mass = 1.0f;
		FVector CenterOfMassLocal = { 0, 0, 0 }; // 컴포넌트 local 좌표계 offset
	};

    std::unordered_map<uint32, FBodyState> BodyStates;

	static constexpr float GravityZ = -9.81f;

	// 이전/현재 프레임 오버랩 쌍 — Begin/End 이벤트 diff용
    std::unordered_set<FNativePhysicsPair> PreviousOverlaps;
    std::unordered_set<FNativePhysicsPair> CurrentOverlaps;

	// Block 쌍 추적 — Hit 이벤트는 첫 접촉 시에만 발화
    std::unordered_set<FNativePhysicsPair> PreviousBlockPairs;
    std::unordered_set<FNativePhysicsPair> CurrentBlockPairs;
};
